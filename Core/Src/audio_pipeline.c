#include "audio_pipeline.h"

#include "audio_equalizer.h"
#include "audio_echo.h"
#include "audio_reverb.h"
#include "noise_reduction.h"

#include <math.h>
#include <stddef.h>

#define LIMITER_THRESHOLD 32000.0f
#define LIMITER_RELEASE 0.05f

static float32_t leftBuffer[AUDIO_PIPELINE_FRAME_COUNT];
static float32_t rightBuffer[AUDIO_PIPELINE_FRAME_COUNT];

static float32_t limiterGain = 1.0f;
static uint8_t equalizerEnabled = 1U;

static void AudioPipeline_ApplyLimiter(void)
{
    float32_t peak = 0.0f;

    for (uint32_t i = 0U;
         i < AUDIO_PIPELINE_FRAME_COUNT;
         i++)
    {
        float32_t leftPeak = fabsf(leftBuffer[i]);
        float32_t rightPeak = fabsf(rightBuffer[i]);

        if (leftPeak > peak)
        {
            peak = leftPeak;
        }

        if (rightPeak > peak)
        {
            peak = rightPeak;
        }
    }

    float32_t targetGain = 1.0f;

    if (peak > LIMITER_THRESHOLD)
    {
        targetGain = LIMITER_THRESHOLD / peak;
    }

    if (targetGain < limiterGain)
    {
        limiterGain = targetGain;
    }
    else
    {
        limiterGain +=
            LIMITER_RELEASE * (targetGain - limiterGain);
    }

    for (uint32_t i = 0U;
         i < AUDIO_PIPELINE_FRAME_COUNT;
         i++)
    {
        leftBuffer[i] *= limiterGain;
        rightBuffer[i] *= limiterGain;
    }
}

void AudioPipeline_Init(float32_t sampleRate)
{
    limiterGain = 1.0f;

    AudioEqualizer_Init(sampleRate);
    AudioEcho_Init(sampleRate);
    AudioReverb_Init(sampleRate);
    AudioNoiseReduction_Init();
}

void AudioPipeline_Process(uint8_t *audioData)
{
    if (audioData == NULL)
    {
        return;
    }

    if ((equalizerEnabled == 0U) &&
        (AudioEcho_IsEnabled() == 0U) &&
        (AudioReverb_IsEnabled() == 0U) &&
        (AudioNoiseReduction_IsEnabled() == 0U))
    {
        return;
    }

    int16_t *samples = (int16_t *)audioData;

    /* Convert interleaved stereo PCM16 to separate float buffers. */
    for (uint32_t i = 0U;
         i < AUDIO_PIPELINE_FRAME_COUNT;
         i++)
    {
        leftBuffer[i] =
            (float32_t)samples[2U * i];

        rightBuffer[i] =
            (float32_t)samples[2U * i + 1U];
    }

    /*
     * Processing order:
     * Noise Reduction -> Equalizer -> Echo -> Reverb -> Limiter
     */
    if (AudioNoiseReduction_IsEnabled() != 0U)
    {
        AudioNoiseReduction_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );
    }

    if (equalizerEnabled != 0U)
    {
        AudioEqualizer_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );
    }

    if (AudioEcho_IsEnabled() != 0U)
    {
        AudioEcho_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );
    }

    if (AudioReverb_IsEnabled() != 0U)
    {
        AudioReverb_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );
    }

    AudioPipeline_ApplyLimiter();

    /* Saturate and convert the processed samples back to PCM16. */
    for (uint32_t i = 0U;
         i < AUDIO_PIPELINE_FRAME_COUNT;
         i++)
    {
        float32_t left = leftBuffer[i];
        float32_t right = rightBuffer[i];

        if (left > 32767.0f)
        {
            left = 32767.0f;
        }
        else if (left < -32768.0f)
        {
            left = -32768.0f;
        }

        if (right > 32767.0f)
        {
            right = 32767.0f;
        }
        else if (right < -32768.0f)
        {
            right = -32768.0f;
        }

        samples[2U * i] = (int16_t)left;
        samples[2U * i + 1U] = (int16_t)right;
    }
}
