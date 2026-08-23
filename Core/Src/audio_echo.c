#include "audio_echo.h"

#include "audio_benchmark.h"

#include <stddef.h>
#include <string.h>

#define ECHO_BUFFER_ADDRESS 0xC0100000U
#define ECHO_MAX_SAMPLE_RATE 48000U
#define ECHO_MAX_DELAY_MS 500U
#define ECHO_DELAY_MS 280U
#define ECHO_CHANNEL_COUNT 2U
#define ECHO_MIX 0.28f
#define ECHO_FEEDBACK 0.37f

#define ECHO_MAX_DELAY_SAMPLES \
    ((ECHO_MAX_SAMPLE_RATE * ECHO_MAX_DELAY_MS) / 1000U)
#define ECHO_BUFFER_FLOAT_COUNT \
    (ECHO_MAX_DELAY_SAMPLES * ECHO_CHANNEL_COUNT)

static float32_t *const echoBuffer =
    (float32_t *)ECHO_BUFFER_ADDRESS;

static uint32_t bufferIndex;
static uint32_t delaySamples = 1U;
static volatile uint8_t enabled;
static volatile uint8_t resetPending;

static void AudioEcho_Reset(void)
{
    bufferIndex = 0U;

    memset(
        echoBuffer,
        0,
        ECHO_BUFFER_FLOAT_COUNT * sizeof(float32_t)
    );
}

void AudioEcho_Init(float32_t sampleRate)
{
    uint32_t requestedSamples =
        (uint32_t)((sampleRate * ECHO_DELAY_MS) / 1000.0f);

    if (requestedSamples < 1U)
    {
        requestedSamples = 1U;
    }
    else if (requestedSamples > ECHO_MAX_DELAY_SAMPLES)
    {
        requestedSamples = ECHO_MAX_DELAY_SAMPLES;
    }

    delaySamples = requestedSamples;
    resetPending = 0U;
    AudioEcho_Reset();
}

void AudioEcho_SetEnabled(uint8_t newEnabled)
{
    uint8_t newState = (newEnabled != 0U) ? 1U : 0U;

    if (enabled != newState)
    {
        enabled = newState;
        resetPending = 1U;
        AudioBenchmark_RequestReset();
    }
}

uint8_t AudioEcho_IsEnabled(void)
{
    return enabled;
}

void AudioEcho_Process(
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
        AudioEcho_Reset();
    }

    if (enabled == 0U)
    {
        return;
    }

    const float32_t dryMix = 1.0f - ECHO_MIX;

    for (uint32_t i = 0U; i < sampleCount; i++)
    {
        uint32_t channelIndex =
            bufferIndex * ECHO_CHANNEL_COUNT;

        float32_t dryLeft = left[i];
        float32_t dryRight = right[i];
        float32_t delayedLeft = echoBuffer[channelIndex];
        float32_t delayedRight = echoBuffer[channelIndex + 1U];

        /* y[n] = (1-M)x[n] + M*d[n] */
        left[i] =
            dryMix * dryLeft + ECHO_MIX * delayedLeft;
        right[i] =
            dryMix * dryRight + ECHO_MIX * delayedRight;

        /* b[n] = x[n] + F*d[n] */
        echoBuffer[channelIndex] =
            dryLeft + ECHO_FEEDBACK * delayedLeft;
        echoBuffer[channelIndex + 1U] =
            dryRight + ECHO_FEEDBACK * delayedRight;

        bufferIndex++;

        if (bufferIndex >= delaySamples)
        {
            bufferIndex = 0U;
        }
    }
}
