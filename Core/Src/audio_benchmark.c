#include "audio_benchmark.h"

#if AUDIO_BENCHMARK_ENABLED

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
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
static uint32_t benchmarkStartTick;

static const char *const stageNames[AUDIO_BENCH_STAGE_COUNT] =
{
    "Pipeline",
    "PCM16->float",
    "Noise reduction",
    "Equalizer",
    "Echo",
    "Reverb",
    "Limiter",
    "float->PCM16",
    "Spectrum"
};

void AudioBenchmark_Init(void)
{
    AudioBenchmark_Reset();
}

void AudioBenchmark_Reset(void)
{
    /* DWT is dedicated to the on-device audio performance counters. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Cortex-M7 debug components can remain locked after reset/debugging. */
    DWT->LAR = 0xC5ACCE55U;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();

    memset(counters, 0, sizeof(counters));
    loadProcessingCycles = 0U;
    loadPreviousCycles = DWT->CYCCNT;
    benchmarkStartTick = HAL_GetTick();
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

void AudioBenchmark_Report(void)
{
    uint32_t elapsedMs = HAL_GetTick() - benchmarkStartTick;
    uint64_t availableCycles;

    if ((elapsedMs == 0U) || (SystemCoreClock == 0U))
    {
        return;
    }

    availableCycles =
        ((uint64_t)SystemCoreClock * elapsedMs) / 1000U;

    printf(
        "\r\nDSP benchmark (%lu ms):\r\n",
        (unsigned long)elapsedMs
    );

    for (uint32_t i = 0U; i < AUDIO_BENCH_STAGE_COUNT; i++)
    {
        uint64_t cycles = counters[i].cycles;
        uint32_t calls = counters[i].calls;
        uint32_t cpuPermille =
            (availableCycles > 0U)
                ? (uint32_t)((cycles * 1000U) / availableCycles)
                : 0U;
        uint32_t averageUs =
            (calls > 0U)
                ? (uint32_t)(
                    ((cycles / calls) * 1000000U) /
                    SystemCoreClock
                  )
                : 0U;
        uint32_t maximumUs = (uint32_t)(
            ((uint64_t)counters[i].maximumCycles * 1000000U) /
            SystemCoreClock
        );

        printf(
            "%-17s %3lu.%lu%%  avg=%lu us  max=%lu us  calls=%lu\r\n",
            stageNames[i],
            (unsigned long)(cpuPermille / 10U),
            (unsigned long)(cpuPermille % 10U),
            (unsigned long)averageUs,
            (unsigned long)maximumUs,
            (unsigned long)calls
        );
    }
}

#endif /* AUDIO_BENCHMARK_ENABLED */
