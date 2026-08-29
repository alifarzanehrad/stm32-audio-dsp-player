#include "system_metrics.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "audio_benchmark.h"
#include "audio_player.h"

#define SYSTEM_METRICS_MAX_TASKS 16U
#define SYSTEM_METRICS_RUN_TIME_DIVIDER 1000U

static uint32_t previousTotalRunTime;
static uint32_t previousIdleRunTime;
static uint32_t cpuLoadPercent;
static uint32_t previousCycleCounter;
static uint32_t dividedRunTimeCounter;
static uint32_t dividedCycleRemainder;

void SystemMetrics_ConfigureRunTimeStats(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    previousCycleCounter = 0U;
    dividedRunTimeCounter = 0U;
    dividedCycleRemainder = 0U;
}

uint32_t SystemMetrics_GetRunTimeCounter(void)
{
    /*
     * FreeRTOS runtime statistics do not handle timer wraparound. Returning
     * the 200 MHz CYCCNT directly wraps in about 21 seconds. A divided counter
     * retains ample resolution and extends that interval to several hours.
     */
    uint32_t currentCycles = DWT->CYCCNT;
    uint32_t elapsedCycles = currentCycles - previousCycleCounter;
    uint64_t accumulatedCycles =
        (uint64_t)dividedCycleRemainder + elapsedCycles;

    previousCycleCounter = currentCycles;
    dividedRunTimeCounter += (uint32_t)(
        accumulatedCycles / SYSTEM_METRICS_RUN_TIME_DIVIDER
    );
    dividedCycleRemainder = (uint32_t)(
        accumulatedCycles % SYSTEM_METRICS_RUN_TIME_DIVIDER
    );

    return dividedRunTimeCounter;
}

static void SystemMetrics_UpdateCpuLoad(void)
{
    TaskStatus_t taskStatus[SYSTEM_METRICS_MAX_TASKS];
    uint32_t totalRunTime = 0U;
    uint32_t idleRunTime = previousIdleRunTime;

    UBaseType_t taskCount = uxTaskGetSystemState(
        taskStatus,
        SYSTEM_METRICS_MAX_TASKS,
        &totalRunTime
    );

    if (taskCount == 0U)
    {
        return;
    }

    TaskHandle_t idleTask = xTaskGetIdleTaskHandle();

    for (UBaseType_t i = 0U; i < taskCount; i++)
    {
        if (taskStatus[i].xHandle == idleTask)
        {
            idleRunTime = taskStatus[i].ulRunTimeCounter;
            break;
        }
    }

    uint32_t totalDelta = totalRunTime - previousTotalRunTime;
    uint32_t idleDelta = idleRunTime - previousIdleRunTime;

    if (totalDelta > 0U)
    {
        if (idleDelta > totalDelta)
        {
            idleDelta = totalDelta;
        }

        cpuLoadPercent =
            100U - (uint32_t)(((uint64_t)idleDelta * 100U) / totalDelta);
    }

    previousTotalRunTime = totalRunTime;
    previousIdleRunTime = idleRunTime;
}

void SystemMetrics_GetSnapshot(SystemMetricsSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    SystemMetrics_UpdateCpuLoad();

    snapshot->cpuLoadPercent = cpuLoadPercent;
    snapshot->dspMaximumUs = AudioBenchmark_GetMaximumProcessingUs();
    snapshot->deadlineMisses = AudioPlayer_GetDeadlineMisses();
    snapshot->freeHeapBytes = (uint32_t)xPortGetFreeHeapSize();
}
