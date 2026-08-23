#include "audio_pipeline.h"

#include "audio_benchmark.h"

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
    AudioBenchmark_Init();

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

    uint32_t pipelineStart = AudioBenchmark_Start();
    int16_t *samples = (int16_t *)audioData;

    uint32_t stageStart = AudioBenchmark_Start();

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

    AudioBenchmark_End(
        AUDIO_BENCH_INPUT_CONVERSION,
        stageStart
    );

    /*
     * Processing order:
     * Noise Reduction -> Equalizer -> Echo -> Reverb -> Limiter
     */
    if (AudioNoiseReduction_IsEnabled() != 0U)
    {
        stageStart = AudioBenchmark_Start();

        AudioNoiseReduction_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );

        AudioBenchmark_End(
            AUDIO_BENCH_NOISE_REDUCTION,
            stageStart
        );
    }

    if (equalizerEnabled != 0U)
    {
        stageStart = AudioBenchmark_Start();

        AudioEqualizer_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );

        AudioBenchmark_End(
            AUDIO_BENCH_EQUALIZER,
            stageStart
        );
    }

    if (AudioEcho_IsEnabled() != 0U)
    {
        stageStart = AudioBenchmark_Start();

        AudioEcho_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );

        AudioBenchmark_End(
            AUDIO_BENCH_ECHO,
            stageStart
        );
    }

    if (AudioReverb_IsEnabled() != 0U)
    {
        stageStart = AudioBenchmark_Start();

        AudioReverb_Process(
            leftBuffer,
            rightBuffer,
            AUDIO_PIPELINE_FRAME_COUNT
        );

        AudioBenchmark_End(
            AUDIO_BENCH_REVERB,
            stageStart
        );
    }

    stageStart = AudioBenchmark_Start();
    AudioPipeline_ApplyLimiter();
    AudioBenchmark_End(AUDIO_BENCH_LIMITER, stageStart);

    stageStart = AudioBenchmark_Start();

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

    AudioBenchmark_End(
        AUDIO_BENCH_OUTPUT_CONVERSION,
        stageStart
    );

    AudioBenchmark_End(AUDIO_BENCH_PIPELINE, pipelineStart);
}
