#include "audio_benchmark.h"

#if AUDIO_BENCHMARK_ENABLED

#include "main.h"

#include <string.h>

typedef struct
{
    uint64_t cycles;
    uint32_t calls;
    uint32_t maximumCycles;
} AudioBenchmarkCounter;

static AudioBenchmarkCounter counters[AUDIO_BENCH_STAGE_COUNT];
static volatile uint8_t resetPending;

void AudioBenchmark_Init(void)
{
    AudioBenchmark_Reset();
}

void AudioBenchmark_Reset(void)
{
    /* DWT is shared with the FreeRTOS CPU-load counter; never reset CYCCNT. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    memset(counters, 0, sizeof(counters));
    resetPending = 0U;
}

void AudioBenchmark_RequestReset(void)
{
    /* Called from the TouchGFX task; reset in the audio task. */
    resetPending = 1U;
}

void AudioBenchmark_ApplyPendingReset(void)
{
    if (resetPending != 0U)
    {
        AudioBenchmark_Reset();
    }
}

uint32_t AudioBenchmark_Start(void)
{
    return DWT->CYCCNT;
}

void AudioBenchmark_End(
    AudioBenchmarkStage stage,
    uint32_t startCycles
)
{
    if (stage >= AUDIO_BENCH_STAGE_COUNT)
    {
        return;
    }

    /* Unsigned subtraction also handles a CYCCNT wraparound. */
    uint32_t elapsedCycles =
        (uint32_t)(DWT->CYCCNT - startCycles);

    counters[stage].cycles += elapsedCycles;
    counters[stage].calls++;

    if (elapsedCycles > counters[stage].maximumCycles)
    {
        counters[stage].maximumCycles = elapsedCycles;
    }
}

uint32_t AudioBenchmark_GetMaximumProcessingUs(void)
{
    if (SystemCoreClock == 0U)
    {
        return 0U;
    }

    uint64_t maximumCycles =
        (uint64_t)counters[AUDIO_BENCH_PIPELINE].maximumCycles +
        (uint64_t)counters[AUDIO_BENCH_SPECTRUM].maximumCycles;

    return (uint32_t)(
        (maximumCycles * 1000000U) / SystemCoreClock
    );
}

#endif /* AUDIO_BENCHMARK_ENABLED */
