#include "audio_equalizer.h"
#include "audio_pipeline.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 48000.0f
#define BLOCK_SIZE EQ_BLOCK_SIZE
#define TEST_BLOCKS 32U

static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static float32_t rms(const float32_t *samples, uint32_t count)
{
    double power = 0.0;

    for (uint32_t i = 0U; i < count; i++)
    {
        power += (double)samples[i] * (double)samples[i];
    }

    return (float32_t)sqrt(power / (double)count);
}

static float32_t measureEqualizerGain(float32_t frequency)
{
    float32_t left[BLOCK_SIZE];
    float32_t right[BLOCK_SIZE];
    float32_t input[BLOCK_SIZE];
    float32_t phase = 0.0f;
    const float32_t phaseStep = 2.0f * PI * frequency / SAMPLE_RATE;
    float32_t inputRms = 0.0f;
    float32_t outputRms = 0.0f;

    AudioEqualizer_Init(SAMPLE_RATE);

    for (uint32_t block = 0U; block < TEST_BLOCKS; block++)
    {
        for (uint32_t i = 0U; i < BLOCK_SIZE; i++)
        {
            input[i] = 1000.0f * sinf(phase);
            left[i] = input[i];
            right[i] = input[i];

            phase += phaseStep;
            if (phase >= 2.0f * PI)
            {
                phase -= 2.0f * PI;
            }
        }

        AudioEqualizer_Process(left, right, BLOCK_SIZE);

        if (block == (TEST_BLOCKS - 1U))
        {
            inputRms = rms(input, BLOCK_SIZE);
            outputRms = rms(left, BLOCK_SIZE);
        }
    }

    return outputRms / inputRms;
}

static void testFlatEqualizer(void)
{
    float32_t left[BLOCK_SIZE];
    float32_t right[BLOCK_SIZE];
    float32_t original[BLOCK_SIZE];

    for (uint32_t band = 0U; band < EQ_BAND_COUNT; band++)
    {
        AudioEQ_SetBandGain((uint8_t)band, 0.0f);
    }

    AudioEqualizer_Init(SAMPLE_RATE);

    for (uint32_t i = 0U; i < BLOCK_SIZE; i++)
    {
        original[i] = 1200.0f * sinf(2.0f * PI * 1000.0f * i / SAMPLE_RATE);
        left[i] = original[i];
        right[i] = original[i];
    }

    AudioEqualizer_Process(left, right, BLOCK_SIZE);

    for (uint32_t i = 0U; i < BLOCK_SIZE; i++)
    {
        require(fabsf(left[i] - original[i]) < 0.1f,
                "flat EQ must preserve the input");
        require(isfinite(left[i]) && isfinite(right[i]),
                "EQ output must remain finite");
    }
}

static void testEqualizerBandResponseAndClamp(void)
{
    AudioEQ_SetBandGain(2U, 100.0f); /* Must clamp to +12 dB. */
    const float32_t centerGain = measureEqualizerGain(1000.0f);
    const float32_t offBandGain = measureEqualizerGain(100.0f);

    require(centerGain > offBandGain * 1.7f,
            "1 kHz boost must exceed the off-band response");
    require(centerGain > 1.6f && centerGain < 2.4f,
            "clamped +12 dB band with headroom should be near +6 dB net");

    AudioEQ_SetBandGain(2U, 0.0f);
}

static void testPipelineLimiter(void)
{
    int16_t samples[2U * AUDIO_PIPELINE_FRAME_COUNT];

    AudioPipeline_Init(SAMPLE_RATE);

    for (uint32_t i = 0U; i < 2U * AUDIO_PIPELINE_FRAME_COUNT; i++)
    {
        samples[i] = INT16_MAX;
    }

    AudioPipeline_Process((uint8_t *)samples);

    for (uint32_t i = 0U; i < 2U * AUDIO_PIPELINE_FRAME_COUNT; i++)
    {
        require(samples[i] <= 32000,
                "limiter output must not exceed its positive threshold");
        require(samples[i] >= -32000,
                "limiter output must not exceed its negative threshold");
    }

    AudioPipeline_Process(NULL);
}

int main(void)
{
    testFlatEqualizer();
    testEqualizerBandResponseAndClamp();
    testPipelineLimiter();

    puts("PASS: DSP host regression tests");
    return EXIT_SUCCESS;
}
