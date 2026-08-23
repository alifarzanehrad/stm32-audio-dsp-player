#include "noise_reduction.h"

#include "audio_benchmark.h"

#include "main.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#define NR_FRAME_SIZE 512U
#define NR_HOP_SIZE (NR_FRAME_SIZE / 2U)
#define NR_BIN_COUNT (NR_FRAME_SIZE / 2U + 1U)
#define NR_INIT_FRAMES 12U
#define NR_OVERSUBTRACTION 2.0f
#define NR_MIN_GAIN 0.08f
#define NR_NOISE_SMOOTHING 0.97f
#define NR_GAIN_SMOOTHING 0.60f
#define NR_NOISE_UPDATE_RATIO 2.5f
#define NR_EPSILON 1.0e-12f

typedef struct
{
    float32_t inputFrame[NR_FRAME_SIZE];
    float32_t outputOverlap[NR_FRAME_SIZE];
    float32_t noisePower[NR_BIN_COUNT];
    float32_t previousGain[NR_BIN_COUNT];
    uint32_t initFrameCount;
} NoiseReductionChannel;

/*
 * Estimate and suppress common-mode noise on the Mid channel only.
 * The Side channel is delayed by one hop so it stays aligned with the
 * overlap-add latency of the processed Mid signal.
 */
static NoiseReductionChannel midChannel;
static float32_t sideDelay[NR_HOP_SIZE];
static uint32_t sideDelayIndex;

static float32_t window[NR_FRAME_SIZE];
static float32_t fftInput[NR_FRAME_SIZE];
static float32_t fftOutput[NR_FRAME_SIZE];
static float32_t currentGain[NR_BIN_COUNT];

static arm_rfft_fast_instance_f32 fftInstance;

static volatile uint8_t enabled;
static volatile uint8_t resetPending;

static void AudioNoiseReduction_Reset(void)
{
    memset(&midChannel, 0, sizeof(midChannel));
    memset(sideDelay, 0, sizeof(sideDelay));
    sideDelayIndex = 0U;

    for (uint32_t k = 0U; k < NR_BIN_COUNT; k++)
    {
        midChannel.previousGain[k] = 1.0f;
    }
}

void AudioNoiseReduction_Init(void)
{
    if (arm_rfft_fast_init_f32(
            &fftInstance,
            NR_FRAME_SIZE
        ) != ARM_MATH_SUCCESS)
    {
        Error_Handler();
    }

    for (uint32_t i = 0U; i < NR_FRAME_SIZE; i++)
    {
        /* Square-root Hann gives perfect 50% overlap reconstruction. */
        float32_t hann =
            0.5f -
            0.5f * cosf((2.0f * PI * i) / NR_FRAME_SIZE);

        window[i] = sqrtf(hann);
    }

    resetPending = 0U;
    AudioNoiseReduction_Reset();
}

void AudioNoiseReduction_SetEnabled(uint8_t newEnabled)
{
    uint8_t newState = (newEnabled != 0U) ? 1U : 0U;

    if (enabled != newState)
    {
        enabled = newState;
        resetPending = 1U;
        AudioBenchmark_RequestReset();
    }
}

uint8_t AudioNoiseReduction_IsEnabled(void)
{
    return enabled;
}

static void AudioNoiseReduction_ProcessChannel(
    NoiseReductionChannel *channel,
    float32_t *samples,
    uint32_t sampleCount
)
{
    for (uint32_t offset = 0U;
         offset < sampleCount;
         offset += NR_HOP_SIZE)
    {
        memmove(
            channel->inputFrame,
            &channel->inputFrame[NR_HOP_SIZE],
            NR_HOP_SIZE * sizeof(float32_t)
        );

        memcpy(
            &channel->inputFrame[NR_HOP_SIZE],
            &samples[offset],
            NR_HOP_SIZE * sizeof(float32_t)
        );

        for (uint32_t i = 0U; i < NR_FRAME_SIZE; i++)
        {
            fftInput[i] =
                channel->inputFrame[i] * window[i];
        }

        arm_rfft_fast_f32(
            &fftInstance,
            fftInput,
            fftOutput,
            0
        );

        /*
         * P[k] = Re(X[k])^2 + Im(X[k])^2
         * N[k] = beta*N[k] + (1-beta)*P[k]
         * G[k] = max(Gmin, 1-alpha*N[k]/(P[k]+epsilon))
         * Y[k] = G[k]*X[k]
         */
        for (uint32_t k = 0U; k < NR_BIN_COUNT; k++)
        {
            float32_t real;
            float32_t imag = 0.0f;

            if (k == 0U)
            {
                real = fftOutput[0];
            }
            else if (k == NR_FRAME_SIZE / 2U)
            {
                real = fftOutput[1];
            }
            else
            {
                real = fftOutput[2U * k];
                imag = fftOutput[2U * k + 1U];
            }

            float32_t power = real * real + imag * imag;

            if (channel->initFrameCount < NR_INIT_FRAMES)
            {
                channel->noisePower[k] +=
                    power / (float32_t)NR_INIT_FRAMES;

                currentGain[k] = 1.0f;
            }
            else
            {
                float32_t noise = channel->noisePower[k];

                if (power < NR_NOISE_UPDATE_RATIO * noise)
                {
                    noise =
                        NR_NOISE_SMOOTHING * noise +
                        (1.0f - NR_NOISE_SMOOTHING) * power;

                    channel->noisePower[k] = noise;
                }

                float32_t gain =
                    1.0f -
                    NR_OVERSUBTRACTION * noise /
                    (power + NR_EPSILON);

                if (gain < NR_MIN_GAIN)
                {
                    gain = NR_MIN_GAIN;
                }
                else if (gain > 1.0f)
                {
                    gain = 1.0f;
                }

                currentGain[k] = gain;
            }
        }

        if (channel->initFrameCount < NR_INIT_FRAMES)
        {
            channel->initFrameCount++;
        }
        else
        {
            for (uint32_t k = 0U; k < NR_BIN_COUNT; k++)
            {
                uint32_t previousBin = (k > 0U) ? k - 1U : k;
                uint32_t nextBin =
                    (k + 1U < NR_BIN_COUNT) ? k + 1U : k;

                float32_t frequencySmoothedGain =
                    (
                        currentGain[previousBin] +
                        2.0f * currentGain[k] +
                        currentGain[nextBin]
                    ) * 0.25f;

                float32_t gain =
                    NR_GAIN_SMOOTHING * channel->previousGain[k] +
                    (1.0f - NR_GAIN_SMOOTHING) *
                    frequencySmoothedGain;

                channel->previousGain[k] = gain;

                if (k == 0U)
                {
                    fftOutput[0] *= gain;
                }
                else if (k == NR_FRAME_SIZE / 2U)
                {
                    fftOutput[1] *= gain;
                }
                else
                {
                    fftOutput[2U * k] *= gain;
                    fftOutput[2U * k + 1U] *= gain;
                }
            }
        }

        arm_rfft_fast_f32(
            &fftInstance,
            fftOutput,
            fftInput,
            1
        );

        for (uint32_t i = 0U; i < NR_FRAME_SIZE; i++)
        {
            channel->outputOverlap[i] +=
                fftInput[i] * window[i];
        }

        memcpy(
            &samples[offset],
            channel->outputOverlap,
            NR_HOP_SIZE * sizeof(float32_t)
        );

        memmove(
            channel->outputOverlap,
            &channel->outputOverlap[NR_HOP_SIZE],
            NR_HOP_SIZE * sizeof(float32_t)
        );

        memset(
            &channel->outputOverlap[NR_HOP_SIZE],
            0,
            NR_HOP_SIZE * sizeof(float32_t)
        );
    }
}

void AudioNoiseReduction_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
)
{
    if ((left == NULL) ||
        (right == NULL) ||
        (sampleCount == 0U) ||
        ((sampleCount % NR_HOP_SIZE) != 0U))
    {
        return;
    }

    if (resetPending != 0U)
    {
        resetPending = 0U;
        AudioNoiseReduction_Reset();
    }

    if (enabled == 0U)
    {
        return;
    }

    /*
     * Mid/Side transform:
     *   M[n] = 0.5 * (L[n] + R[n])
     *   S[n] = 0.5 * (L[n] - R[n])
     *
     * Most background noise and mono speech are common to both channels, so
     * spectral subtraction is performed once on M instead of independently
     * on L and R. This nearly halves the FFT workload.
     */
    for (uint32_t i = 0U; i < sampleCount; i++)
    {
        float32_t inputLeft = left[i];
        float32_t inputRight = right[i];
        float32_t side =
            0.5f * (inputLeft - inputRight);

        left[i] =
            0.5f * (inputLeft + inputRight);

        /*
         * The STFT overlap-add path delays Mid by NR_HOP_SIZE samples.
         * Apply the same delay to Side before stereo reconstruction.
         */
        right[i] = sideDelay[sideDelayIndex];
        sideDelay[sideDelayIndex] = side;

        sideDelayIndex++;

        if (sideDelayIndex >= NR_HOP_SIZE)
        {
            sideDelayIndex = 0U;
        }
    }

    AudioNoiseReduction_ProcessChannel(
        &midChannel,
        left,
        sampleCount
    );

    /*
     * Stereo reconstruction:
     *   L[n] = M[n] + S[n]
     *   R[n] = M[n] - S[n]
     */
    for (uint32_t i = 0U; i < sampleCount; i++)
    {
        float32_t mid = left[i];
        float32_t side = right[i];

        left[i] = mid + side;
        right[i] = mid - side;
    }
}
