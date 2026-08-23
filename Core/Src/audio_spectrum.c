#include "audio_spectrum.h"

#include "audio_benchmark.h"

#include "main.h"
#include <math.h>
#include <stddef.h>

static float32_t fftBands[AUDIO_SPECTRUM_BAND_COUNT];
float32_t fftBandsSmoothed[AUDIO_SPECTRUM_BAND_COUNT];

static float32_t fftInput[AUDIO_SPECTRUM_FFT_SIZE];
static float32_t fftOutput[AUDIO_SPECTRUM_FFT_SIZE];
static float32_t fftMagnitude[AUDIO_SPECTRUM_FFT_SIZE / 2U];
static float32_t hannWindow[AUDIO_SPECTRUM_FFT_SIZE];

static arm_rfft_fast_instance_f32 fftInstance;

void AudioSpectrum_Init(void)
{
    if (arm_rfft_fast_init_f32(
            &fftInstance,
            AUDIO_SPECTRUM_FFT_SIZE
        ) != ARM_MATH_SUCCESS)
    {
        Error_Handler();
    }

    for (uint32_t i = 0U;
         i < AUDIO_SPECTRUM_FFT_SIZE;
         i++)
    {
        hannWindow[i] =
            0.5f *
            (
                1.0f -
                cosf(
                    (2.0f * PI * i) /
                    (AUDIO_SPECTRUM_FFT_SIZE - 1U)
                )
            );
    }
}

void AudioFFT_Process(uint8_t *audioData)
{
    if (audioData == NULL)
    {
        return;
    }

    uint32_t benchmarkStart = AudioBenchmark_Start();
    int16_t *samples = (int16_t *)audioData;

    /* Convert stereo PCM16 to a mono analysis signal. */
    for (uint32_t i = 0U;
         i < AUDIO_SPECTRUM_FFT_SIZE;
         i++)
    {
        int32_t left = samples[2U * i];
        int32_t right = samples[2U * i + 1U];

        fftInput[i] =
            ((float32_t)left + (float32_t)right) * 0.5f;
    }

    /* Remove DC before applying the analysis window. */
    float32_t mean = 0.0f;

    for (uint32_t i = 0U;
         i < AUDIO_SPECTRUM_FFT_SIZE;
         i++)
    {
        mean += fftInput[i];
    }

    mean /= (float32_t)AUDIO_SPECTRUM_FFT_SIZE;

    for (uint32_t i = 0U;
         i < AUDIO_SPECTRUM_FFT_SIZE;
         i++)
    {
        fftInput[i] =
            (fftInput[i] - mean) * hannWindow[i];
    }

    arm_rfft_fast_f32(
        &fftInstance,
        fftInput,
        fftOutput,
        0
    );

    fftMagnitude[0] =
        fabsf(fftOutput[0]) /
        (float32_t)AUDIO_SPECTRUM_FFT_SIZE;

    for (uint32_t k = 1U;
         k < AUDIO_SPECTRUM_FFT_SIZE / 2U;
         k++)
    {
        float32_t real = fftOutput[2U * k];
        float32_t imag = fftOutput[2U * k + 1U];

        fftMagnitude[k] =
            sqrtf(real * real + imag * imag) /
            (float32_t)AUDIO_SPECTRUM_FFT_SIZE;
    }

    /* Non-linear bin ranges provide more resolution at low frequencies. */
    static const uint16_t bandEdges[
        AUDIO_SPECTRUM_BAND_COUNT + 1U
    ] =
    {
        1U,
        2U,
        3U,
        4U,
        6U,
        9U,
        13U,
        19U,
        28U,
        41U,
        60U,
        88U,
        129U,
        189U,
        277U,
        405U,
        511U
    };

    for (uint32_t band = 0U;
         band < AUDIO_SPECTRUM_BAND_COUNT;
         band++)
    {
        float32_t sum = 0.0f;
        uint32_t startBin = bandEdges[band];
        uint32_t endBin = bandEdges[band + 1U];

        for (uint32_t k = startBin; k < endBin; k++)
        {
            sum += fftMagnitude[k];
        }

        uint32_t binCount = endBin - startBin;

        fftBands[band] =
            (binCount > 0U)
                ? sum / (float32_t)binCount
                : 0.0f;
    }

    float32_t maximumBand = 1.0f;

    for (uint32_t band = 0U;
         band < AUDIO_SPECTRUM_BAND_COUNT;
         band++)
    {
        if (fftBands[band] > maximumBand)
        {
            maximumBand = fftBands[band];
        }
    }

    for (uint32_t band = 0U;
         band < AUDIO_SPECTRUM_BAND_COUNT;
         band++)
    {
        float32_t normalized =
            (fftBands[band] / maximumBand) * 100.0f;

        if (normalized > 100.0f)
        {
            normalized = 100.0f;
        }

        /* Temporal smoothing reduces visible bar flicker. */
        fftBandsSmoothed[band] =
            0.7f * fftBandsSmoothed[band] +
            0.3f * normalized;
    }

    AudioBenchmark_End(AUDIO_BENCH_SPECTRUM, benchmarkStart);
}
