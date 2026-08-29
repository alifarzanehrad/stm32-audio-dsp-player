#ifndef AUDIO_BENCHMARK_H
#define AUDIO_BENCHMARK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime counters feed the on-device System Information screen. */
#ifndef AUDIO_BENCHMARK_ENABLED
#define AUDIO_BENCHMARK_ENABLED 1U
#endif

typedef enum
{
    AUDIO_BENCH_PIPELINE = 0,
    AUDIO_BENCH_INPUT_CONVERSION,
    AUDIO_BENCH_NOISE_REDUCTION,
    AUDIO_BENCH_EQUALIZER,
    AUDIO_BENCH_ECHO,
    AUDIO_BENCH_REVERB,
    AUDIO_BENCH_LIMITER,
    AUDIO_BENCH_OUTPUT_CONVERSION,
    AUDIO_BENCH_SPECTRUM,
    AUDIO_BENCH_STAGE_COUNT
} AudioBenchmarkStage;

#if AUDIO_BENCHMARK_ENABLED

void AudioBenchmark_Init(void);
void AudioBenchmark_Reset(void);
void AudioBenchmark_RequestReset(void);
void AudioBenchmark_ApplyPendingReset(void);
uint32_t AudioBenchmark_Start(void);
void AudioBenchmark_End(AudioBenchmarkStage stage, uint32_t startCycles);
uint32_t AudioBenchmark_GetMaximumProcessingUs(void);
uint32_t AudioBenchmark_GetLoadPercent(void);
void AudioBenchmark_Report(void);

#else

#define AudioBenchmark_Init() ((void)0)
#define AudioBenchmark_Reset() ((void)0)
#define AudioBenchmark_RequestReset() ((void)0)
#define AudioBenchmark_ApplyPendingReset() ((void)0)
#define AudioBenchmark_Start() (0U)
#define AudioBenchmark_End(stage, startCycles) ((void)0)
#define AudioBenchmark_GetMaximumProcessingUs() (0U)
#define AudioBenchmark_GetLoadPercent() (0U)
#define AudioBenchmark_Report() ((void)0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_BENCHMARK_H */
