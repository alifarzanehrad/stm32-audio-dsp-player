#include "system_metrics.h"

#include "FreeRTOS.h"
#include "task.h"
#include "audio_benchmark.h"
#include "audio_player.h"
#include "cmsis_os.h"

extern osThreadId defaultTaskHandle;

void SystemMetrics_GetSnapshot(SystemMetricsSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    snapshot->cpuLoadPercent = AudioBenchmark_GetLoadPercent();
    snapshot->dspMaximumUs = AudioBenchmark_GetMaximumProcessingUs();
    snapshot->deadlineMisses = AudioPlayer_GetDeadlineMisses();
    snapshot->freeHeapBytes = (uint32_t)xPortGetFreeHeapSize();
    snapshot->audioStackFreeBytes =
        (uint32_t)uxTaskGetStackHighWaterMark(
            (TaskHandle_t)defaultTaskHandle
        ) * (uint32_t)sizeof(StackType_t);
}
