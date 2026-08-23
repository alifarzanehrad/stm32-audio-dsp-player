#include "audio_benchmark.h"

#if AUDIO_BENCHMARK_ENABLED

#include "main.h"

#include <stdio.h>
#include <string.h>

#define AUDIO_BENCHMARK_REPORT_MS 1000U

typedef struct
{
    uint64_t cycles;
    uint32_t calls;
} AudioBenchmarkCounter;

static AudioBenchmarkCounter counters[AUDIO_BENCH_STAGE_COUNT];
static uint32_t previousReportTick;
static volatile uint8_t resetPending;

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
    /* Resume may leave trace counting disabled, so arm it again. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;

    memset(counters, 0, sizeof(counters));
    previousReportTick = HAL_GetTick();
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
    counters[stage].cycles +=
        (uint32_t)(DWT->CYCCNT - startCycles);

    counters[stage].calls++;
}

void AudioBenchmark_Report(void)
{
    uint32_t currentTick = HAL_GetTick();
    uint32_t elapsedMs = currentTick - previousReportTick;

    if (elapsedMs < AUDIO_BENCHMARK_REPORT_MS)
    {
        return;
    }

    uint64_t availableCycles =
        ((uint64_t)SystemCoreClock * elapsedMs) / 1000U;

    printf("\r\nDSP benchmark (%lu ms):\r\n",
           (unsigned long)elapsedMs);

    for (uint32_t i = 0U;
         i < AUDIO_BENCH_STAGE_COUNT;
         i++)
    {
        uint64_t cycles = counters[i].cycles;
        uint32_t calls = counters[i].calls;

        uint32_t cpuPermille =
            (availableCycles > 0U)
                ? (uint32_t)((cycles * 1000U) / availableCycles)
                : 0U;

        uint32_t averageUs =
            ((calls > 0U) && (SystemCoreClock > 0U))
                ? (uint32_t)(
                    ((cycles / calls) * 1000000U) /
                    SystemCoreClock
                  )
                : 0U;

        printf(
            "%-17s %3lu.%lu%%  avg=%lu us  calls=%lu\r\n",
            stageNames[i],
            (unsigned long)(cpuPermille / 10U),
            (unsigned long)(cpuPermille % 10U),
            (unsigned long)averageUs,
            (unsigned long)calls
        );
    }

    memset(counters, 0, sizeof(counters));
    previousReportTick = currentTick;
}

#endif /* AUDIO_BENCHMARK_ENABLED */
