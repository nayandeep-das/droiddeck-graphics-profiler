
#ifndef CORRELATION_ANALYZER_HPP
#define CORRELATION_ANALYZER_HPP

#include "wayland_log_parser.h"

#include <cstddef>
#include <optional>
#include <vector>

struct CorrelationResult {
    std::size_t sampleCount = 0;

    // Pearson correlation coefficient (-1 to +1).
    std::optional<double> coefficient;
};

struct CorrelationAnalysis {
    std::size_t gameplaySampleCount = 0;
    std::size_t criticalSampleCount = 0;
    std::size_t higherFpsSampleCount = 0;

    // FPS correlation with individual telemetry metrics.
    CorrelationResult fpsVsGpuFrames;
    CorrelationResult fpsVsFramesOnScreen;
    CorrelationResult fpsVsHeldFrameRatio;
    CorrelationResult fpsVsGpuReleaseWaits;

    // Average values for critical and higher-FPS groups.
    double criticalAverageFps = 0.0;
    double higherAverageFps = 0.0;

    double criticalAverageGpuFrames = 0.0;
    double higherAverageGpuFrames = 0.0;

    double criticalAverageFramesOnScreen = 0.0;
    double higherAverageFramesOnScreen = 0.0;

    double criticalAverageHeldRatio = 0.0;
    double higherAverageHeldRatio = 0.0;

    double criticalAverageGpuReleaseWaits = 0.0;
    double higherAverageGpuReleaseWaits = 0.0;

    std::size_t criticalHeldRatioSamples = 0;
    std::size_t higherHeldRatioSamples = 0;
};

CorrelationAnalysis analyzeCorrelations(
    const std::vector<WaylandLogSample>& samples
);

#endif
