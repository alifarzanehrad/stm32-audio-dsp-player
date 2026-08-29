#include <gui/infoscreen_screen/InfoScreenView.hpp>
#include <touchgfx/Unicode.hpp>

#include <stdint.h>

typedef struct
{
    uint32_t cpuLoadPercent;
    uint32_t dspMaximumUs;
    uint32_t deadlineMisses;
    uint32_t freeHeapBytes;
    uint32_t audioStackFreeBytes;
} SystemMetricsSnapshot;

extern "C" void SystemMetrics_GetSnapshot(
    SystemMetricsSnapshot *snapshot
);

#ifdef SIMULATOR

extern "C" void SystemMetrics_GetSnapshot(
    SystemMetricsSnapshot *snapshot
)
{
    if (snapshot != 0)
    {
        snapshot->cpuLoadPercent = 42U;
        snapshot->dspMaximumUs = 12010U;
        snapshot->deadlineMisses = 0U;
        snapshot->freeHeapBytes = 19920U;
        snapshot->audioStackFreeBytes = 3400U;
    }
}

#endif

InfoScreenView::InfoScreenView()
    : updateTickCounter(0U)
{

}

void InfoScreenView::setupScreen()
{
    InfoScreenViewBase::setupScreen();
    updateMetrics();
}

void InfoScreenView::handleTickEvent()
{
    updateTickCounter++;

    if (updateTickCounter >= 30U)
    {
        updateTickCounter = 0U;
        updateMetrics();
    }
}

void InfoScreenView::updateMetrics()
{
    SystemMetricsSnapshot metrics;
    SystemMetrics_GetSnapshot(&metrics);

    uint32_t dspMaximumMs = (metrics.dspMaximumUs + 500U) / 1000U;
    uint32_t freeHeapKB = metrics.freeHeapBytes / 1024U;
    uint32_t audioStackFreeKB = metrics.audioStackFreeBytes / 1024U;

    if (dspMaximumMs > 999U)
    {
        dspMaximumMs = 999U;
    }

    if (metrics.deadlineMisses > 999U)
    {
        metrics.deadlineMisses = 999U;
    }

    if (freeHeapKB > 999U)
    {
        freeHeapKB = 999U;
    }

    if (audioStackFreeKB > 999U)
    {
        audioStackFreeKB = 999U;
    }

    touchgfx::Unicode::snprintf(
        CpuLoadValueBuffer,
        CPULOADVALUE_SIZE,
        "%u",
        static_cast<unsigned int>(metrics.cpuLoadPercent)
    );
    touchgfx::Unicode::snprintf(
        DspMaxValueBuffer,
        DSPMAXVALUE_SIZE,
        "%u",
        static_cast<unsigned int>(dspMaximumMs)
    );
    touchgfx::Unicode::snprintf(
        DeadlineMissesValueBuffer,
        DEADLINEMISSESVALUE_SIZE,
        "%u",
        static_cast<unsigned int>(metrics.deadlineMisses)
    );
    touchgfx::Unicode::snprintf(
        FreeHeapValueBuffer,
        FREEHEAPVALUE_SIZE,
        "%u",
        static_cast<unsigned int>(freeHeapKB)
    );
    touchgfx::Unicode::snprintf(
        AudioStackValueBuffer,
        AUDIOSTACKVALUE_SIZE,
        "%u",
        static_cast<unsigned int>(audioStackFreeKB)
    );

    CpuLoadValue.invalidate();
    DspMaxValue.invalidate();
    DeadlineMissesValue.invalidate();
    FreeHeapValue.invalidate();
    AudioStackValue.invalidate();
}

void InfoScreenView::handleGestureEvent(
    const touchgfx::GestureEvent& event
)
{
    if ((event.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) &&
        (event.getVelocity() > 0))
    {
        application().gotoEffectsScreen();
        return;
    }

    InfoScreenViewBase::handleGestureEvent(event);
}

void InfoScreenView::tearDownScreen()
{
    InfoScreenViewBase::tearDownScreen();
}
