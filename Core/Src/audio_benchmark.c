#include "audio_benchmark.h"

#if AUDIO_BENCHMARK_ENABLED

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

typedef struct
{
    uint64_t cycles;
    uint32_t calls;
    volatile uint32_t maximumCycles;
} AudioBenchmarkCounter;

static AudioBenchmarkCounter counters[AUDIO_BENCH_STAGE_COUNT];
static volatile uint8_t resetPending;
static volatile uint32_t loadProcessingCycles;
static uint32_t loadPreviousCycles;

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
    loadProcessingCycles = 0U;
    loadPreviousCycles = DWT->CYCCNT;
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

    if ((stage == AUDIO_BENCH_PIPELINE) ||
        (stage == AUDIO_BENCH_SPECTRUM))
    {
        loadProcessingCycles += elapsedCycles;
    }
}

uint32_t AudioBenchmark_GetMaximumProcessingUs(void)
{
    if (SystemCoreClock == 0U)
    {
        return 0U;
    }

    uint32_t pipelineMaximum =
        counters[AUDIO_BENCH_PIPELINE].maximumCycles;
    uint32_t spectrumMaximum =
        counters[AUDIO_BENCH_SPECTRUM].maximumCycles;

    uint64_t maximumCycles =
        (uint64_t)pipelineMaximum + (uint64_t)spectrumMaximum;

    return (uint32_t)(
        (maximumCycles * 1000000U) / SystemCoreClock
    );
}

uint32_t AudioBenchmark_GetLoadPercent(void)
{
    uint32_t currentCycles = DWT->CYCCNT;
    uint32_t elapsedCycles = currentCycles - loadPreviousCycles;
    uint32_t processingCycles;

    taskENTER_CRITICAL();
    processingCycles = loadProcessingCycles;
    loadProcessingCycles = 0U;
    taskEXIT_CRITICAL();

    loadPreviousCycles = currentCycles;

    if (elapsedCycles == 0U)
    {
        return 0U;
    }

    uint32_t loadPercent = (uint32_t)(
        ((uint64_t)processingCycles * 100U) / elapsedCycles
    );

    return (loadPercent > 100U) ? 100U : loadPercent;
}

#endif /* AUDIO_BENCHMARK_ENABLED */
