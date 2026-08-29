#include "system_metrics.h"

#include "FreeRTOS.h"
#include "task.h"
#include "audio_benchmark.h"
#include "audio_player.h"

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
}
