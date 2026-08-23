#include "audio_reverb.h"

#include "audio_benchmark.h"

#include <stddef.h>
#include <string.h>

#define REVERB_BUFFER_ADDRESS 0xC0140000U
#define REVERB_CHANNEL_COUNT 2U
#define REVERB_COMB_COUNT 4U
#define REVERB_ALLPASS_COUNT 2U
#define REVERB_MAX_SAMPLE_RATE 48000U
#define REVERB_COMB_MAX_DELAY_MS 50U
#define REVERB_ALLPASS_MAX_DELAY_MS 8U

#define REVERB_COMB_MAX_SAMPLES \
    ((REVERB_MAX_SAMPLE_RATE * REVERB_COMB_MAX_DELAY_MS) / 1000U)
#define REVERB_ALLPASS_MAX_SAMPLES \
    ((REVERB_MAX_SAMPLE_RATE * REVERB_ALLPASS_MAX_DELAY_MS) / 1000U)

#define REVERB_MIX 0.22f
#define REVERB_FEEDBACK 0.72f
#define REVERB_DAMPING 0.25f
#define REVERB_ALLPASS_GAIN 0.50f

typedef struct
{
    float32_t *buffer;
    uint32_t length;
    uint32_t index;
    float32_t dampingState;
} ReverbDelayLine;

static float32_t *const reverbBuffer =
    (float32_t *)REVERB_BUFFER_ADDRESS;

static ReverbDelayLine
    combFilters[REVERB_CHANNEL_COUNT][REVERB_COMB_COUNT];
static ReverbDelayLine
    allpassFilters[REVERB_CHANNEL_COUNT][REVERB_ALLPASS_COUNT];

static volatile uint8_t enabled;
static volatile uint8_t resetPending;

static void AudioReverb_Reset(void)
{
    const uint32_t combFloatCount =
        REVERB_CHANNEL_COUNT *
        REVERB_COMB_COUNT *
        REVERB_COMB_MAX_SAMPLES;

    const uint32_t allpassFloatCount =
        REVERB_CHANNEL_COUNT *
        REVERB_ALLPASS_COUNT *
        REVERB_ALLPASS_MAX_SAMPLES;

    memset(
        reverbBuffer,
        0,
        (combFloatCount + allpassFloatCount) * sizeof(float32_t)
    );

    for (uint32_t channel = 0U;
         channel < REVERB_CHANNEL_COUNT;
         channel++)
    {
        for (uint32_t stage = 0U;
             stage < REVERB_COMB_COUNT;
             stage++)
        {
            combFilters[channel][stage].index = 0U;
            combFilters[channel][stage].dampingState = 0.0f;
        }

        for (uint32_t stage = 0U;
             stage < REVERB_ALLPASS_COUNT;
             stage++)
        {
            allpassFilters[channel][stage].index = 0U;
            allpassFilters[channel][stage].dampingState = 0.0f;
        }
    }
}

void AudioReverb_Init(float32_t sampleRate)
{
    static const float32_t combDelayMs
        [REVERB_CHANNEL_COUNT][REVERB_COMB_COUNT] =
    {
        {29.7f, 37.1f, 41.1f, 43.7f},
        {30.9f, 38.3f, 42.3f, 44.9f}
    };

    static const float32_t allpassDelayMs
        [REVERB_CHANNEL_COUNT][REVERB_ALLPASS_COUNT] =
    {
        {5.0f, 1.7f},
        {5.5f, 2.1f}
    };

    uint32_t allpassBase =
        REVERB_CHANNEL_COUNT *
        REVERB_COMB_COUNT *
        REVERB_COMB_MAX_SAMPLES;

    for (uint32_t channel = 0U;
         channel < REVERB_CHANNEL_COUNT;
         channel++)
    {
        for (uint32_t stage = 0U;
             stage < REVERB_COMB_COUNT;
             stage++)
        {
            uint32_t slot =
                channel * REVERB_COMB_COUNT + stage;

            uint32_t length = (uint32_t)(
                sampleRate * combDelayMs[channel][stage] / 1000.0f
            );

            if (length < 1U)
            {
                length = 1U;
            }
            else if (length > REVERB_COMB_MAX_SAMPLES)
            {
                length = REVERB_COMB_MAX_SAMPLES;
            }

            combFilters[channel][stage].buffer =
                &reverbBuffer[slot * REVERB_COMB_MAX_SAMPLES];

            combFilters[channel][stage].length = length;
        }

        for (uint32_t stage = 0U;
             stage < REVERB_ALLPASS_COUNT;
             stage++)
        {
            uint32_t slot =
                channel * REVERB_ALLPASS_COUNT + stage;

            uint32_t length = (uint32_t)(
                sampleRate * allpassDelayMs[channel][stage] / 1000.0f
            );

            if (length < 1U)
            {
                length = 1U;
            }
            else if (length > REVERB_ALLPASS_MAX_SAMPLES)
            {
                length = REVERB_ALLPASS_MAX_SAMPLES;
            }

            allpassFilters[channel][stage].buffer =
                &reverbBuffer[
                    allpassBase +
                    slot * REVERB_ALLPASS_MAX_SAMPLES
                ];

            allpassFilters[channel][stage].length = length;
        }
    }

    resetPending = 0U;
    AudioReverb_Reset();
}

void AudioReverb_SetEnabled(uint8_t newEnabled)
{
    uint8_t newState = (newEnabled != 0U) ? 1U : 0U;

    if (enabled != newState)
    {
        enabled = newState;
        resetPending = 1U;
        AudioBenchmark_RequestReset();
    }
}

uint8_t AudioReverb_IsEnabled(void)
{
    return enabled;
}

static float32_t AudioReverb_ProcessComb(
    ReverbDelayLine *line,
    float32_t input
)
{
    float32_t delayed = line->buffer[line->index];

    /* f[n] = (1-D)d[n] + D*f[n-1] */
    line->dampingState =
        (1.0f - REVERB_DAMPING) * delayed +
        REVERB_DAMPING * line->dampingState;

    /* b[n] = x[n] + G*f[n] */
    line->buffer[line->index] =
        input + REVERB_FEEDBACK * line->dampingState;

    line->index++;

    if (line->index >= line->length)
    {
        line->index = 0U;
    }

    return delayed;
}

static float32_t AudioReverb_ProcessAllpass(
    ReverbDelayLine *line,
    float32_t input
)
{
    float32_t delayed = line->buffer[line->index];

    /* y[n] = -G*x[n] + d[n] */
    float32_t output =
        -REVERB_ALLPASS_GAIN * input + delayed;

    /* b[n] = x[n] + G*y[n] */
    line->buffer[line->index] =
        input + REVERB_ALLPASS_GAIN * output;

    line->index++;

    if (line->index >= line->length)
    {
        line->index = 0U;
    }

    return output;
}

void AudioReverb_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
)
{
    if ((left == NULL) || (right == NULL) || (sampleCount == 0U))
    {
        return;
    }

    if (resetPending != 0U)
    {
        resetPending = 0U;
        AudioReverb_Reset();
    }

    if (enabled == 0U)
    {
        return;
    }

    for (uint32_t i = 0U; i < sampleCount; i++)
    {
        float32_t dry[REVERB_CHANNEL_COUNT] =
        {
            left[i],
            right[i]
        };

        float32_t wet[REVERB_CHANNEL_COUNT] =
        {
            0.0f,
            0.0f
        };

        for (uint32_t channel = 0U;
             channel < REVERB_CHANNEL_COUNT;
             channel++)
        {
            for (uint32_t stage = 0U;
                 stage < REVERB_COMB_COUNT;
                 stage++)
            {
                wet[channel] += AudioReverb_ProcessComb(
                    &combFilters[channel][stage],
                    dry[channel]
                );
            }

            wet[channel] /= (float32_t)REVERB_COMB_COUNT;

            for (uint32_t stage = 0U;
                 stage < REVERB_ALLPASS_COUNT;
                 stage++)
            {
                wet[channel] = AudioReverb_ProcessAllpass(
                    &allpassFilters[channel][stage],
                    wet[channel]
                );
            }
        }

        /* output[n] = dry[n] + M*wet[n] */
        left[i] = dry[0] + REVERB_MIX * wet[0];
        right[i] = dry[1] + REVERB_MIX * wet[1];
    }
}
