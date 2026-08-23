#include "audio_equalizer.h"

#include "audio_benchmark.h"

#include <math.h>
#include <stddef.h>

#define BIQUAD_COEFFS_PER_STAGE 5U
#define BIQUAD_STATE_PER_STAGE 4U
#define EQ_MAX_HEADROOM_DB 6.0f

static const float32_t bandFrequencies[EQ_BAND_COUNT] =
{
    100.0f,
    300.0f,
    1000.0f,
    3000.0f,
    8000.0f
};

static float32_t bandGainsDB[EQ_BAND_COUNT];
static volatile float32_t requestedGainsDB[EQ_BAND_COUNT];
static volatile uint8_t updatePending;

static float32_t sampleRate = 48000.0f;
static float32_t preampGain = 1.0f;

static float32_t coefficients[
    EQ_BAND_COUNT * BIQUAD_COEFFS_PER_STAGE
];
static float32_t leftState[
    EQ_BAND_COUNT * BIQUAD_STATE_PER_STAGE
];
static float32_t rightState[
    EQ_BAND_COUNT * BIQUAD_STATE_PER_STAGE
];

static arm_biquad_casd_df1_inst_f32 leftEqualizer;
static arm_biquad_casd_df1_inst_f32 rightEqualizer;

static void AudioEqualizer_CalculateCoefficients(void)
{
    const float32_t Q = 1.0f;

    for (uint32_t band = 0U; band < EQ_BAND_COUNT; band++)
    {
        float32_t gainDB = bandGainsDB[band];
        float32_t A = powf(10.0f, gainDB / 40.0f);
        float32_t omega =
            2.0f * PI * bandFrequencies[band] / sampleRate;
        float32_t alpha = sinf(omega) / (2.0f * Q);
        float32_t cosOmega = cosf(omega);

        float32_t b0 = 1.0f + alpha * A;
        float32_t b1 = -2.0f * cosOmega;
        float32_t b2 = 1.0f - alpha * A;
        float32_t a0 = 1.0f + alpha / A;
        float32_t a1 = -2.0f * cosOmega;
        float32_t a2 = 1.0f - alpha / A;

        uint32_t index = band * BIQUAD_COEFFS_PER_STAGE;

        coefficients[index] = b0 / a0;
        coefficients[index + 1U] = b1 / a0;
        coefficients[index + 2U] = b2 / a0;

        /* CMSIS-DSP DF1 expects inverted feedback signs. */
        coefficients[index + 3U] = -(a1 / a0);
        coefficients[index + 4U] = -(a2 / a0);
    }
}

static void AudioEqualizer_UpdatePreampGain(void)
{
    float32_t maximumBoostDB = 0.0f;

    for (uint32_t band = 0U; band < EQ_BAND_COUNT; band++)
    {
        if (bandGainsDB[band] > maximumBoostDB)
        {
            maximumBoostDB = bandGainsDB[band];
        }
    }

    if (maximumBoostDB > EQ_MAX_HEADROOM_DB)
    {
        maximumBoostDB = EQ_MAX_HEADROOM_DB;
    }

    preampGain = powf(10.0f, -maximumBoostDB / 20.0f);
}

static void AudioEqualizer_ApplyPendingGains(void)
{
    if (updatePending == 0U)
    {
        return;
    }

    updatePending = 0U;

    for (uint32_t band = 0U; band < EQ_BAND_COUNT; band++)
    {
        bandGainsDB[band] = requestedGainsDB[band];
    }

    AudioEqualizer_UpdatePreampGain();
    AudioEqualizer_CalculateCoefficients();
}

void AudioEqualizer_Init(float32_t newSampleRate)
{
    sampleRate = newSampleRate;

    for (uint32_t band = 0U; band < EQ_BAND_COUNT; band++)
    {
        bandGainsDB[band] = requestedGainsDB[band];
    }

    updatePending = 0U;
    AudioEqualizer_UpdatePreampGain();
    AudioEqualizer_CalculateCoefficients();

    arm_biquad_cascade_df1_init_f32(
        &leftEqualizer,
        EQ_BAND_COUNT,
        coefficients,
        leftState
    );

    arm_biquad_cascade_df1_init_f32(
        &rightEqualizer,
        EQ_BAND_COUNT,
        coefficients,
        rightState
    );
}

void AudioEQ_SetBandGain(uint8_t band, float32_t gainDB)
{
    if (band >= EQ_BAND_COUNT)
    {
        return;
    }

    if (gainDB < -12.0f)
    {
        gainDB = -12.0f;
    }
    else if (gainDB > 12.0f)
    {
        gainDB = 12.0f;
    }

    if (requestedGainsDB[band] != gainDB)
    {
        requestedGainsDB[band] = gainDB;
        updatePending = 1U;
        AudioBenchmark_RequestReset();
    }
}

void AudioEqualizer_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
)
{
    if ((left == NULL) || (right == NULL) || (sampleCount == 0U))
    {
        return;
    }

    AudioEqualizer_ApplyPendingGains();

    for (uint32_t i = 0U; i < sampleCount; i++)
    {
        left[i] *= preampGain;
        right[i] *= preampGain;
    }

    arm_biquad_cascade_df1_f32(
        &leftEqualizer,
        left,
        left,
        sampleCount
    );

    arm_biquad_cascade_df1_f32(
        &rightEqualizer,
        right,
        right,
        sampleCount
    );
}
