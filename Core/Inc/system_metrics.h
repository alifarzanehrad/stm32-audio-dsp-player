#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t cpuLoadPercent;
    uint32_t dspMaximumUs;
    uint32_t deadlineMisses;
    uint32_t freeHeapBytes;
} SystemMetricsSnapshot;

void SystemMetrics_GetSnapshot(SystemMetricsSnapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_METRICS_H */
