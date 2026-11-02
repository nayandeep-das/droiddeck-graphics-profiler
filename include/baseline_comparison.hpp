
#ifndef BASELINE_COMPARISON_HPP
#define BASELINE_COMPARISON_HPP

#include "wayland_log_parser.h"

#include <cstddef>
#include <optional>
#include <vector>


struct BaselineMetrics {
    std::size_t sampleCount = 0;

    double averageFps = 0.0;
    double minimumFps = 0.0;
    double maximumFps = 0.0;

    double averageRenderSceneMs = 0.0;
    double averageAcquireMs = 0.0;
    double averagePresentMs = 0.0;
    double averageFenceWaitMs = 0.0;
    double averageReleaseMs = 0.0;

    double maximumReleaseMs = 0.0;

    double averageHeldFrameRatio = 0.0;
    std::size_t heldRatioSampleCount = 0;
};

struct MetricDifference {
    double criticalValue = 0.0;
    double higherValue = 0.0;

    // Critical average minus Higher average.
    double absoluteDifference = 0.0;

    // Percentage relative to the Higher-FPS average.
    // Empty when the Higher-FPS average is zero.
    std::optional<double> percentageDifference;
};

struct BaselineComparison {
    BaselineMetrics critical;
    BaselineMetrics higher;

    MetricDifference fps;
    MetricDifference renderScene;
    MetricDifference acquire;
    MetricDifference present;
    MetricDifference fenceWait;
    MetricDifference release;
};

BaselineComparison compareBaselines(
    const std::vector<WaylandLogSample>& samples
);

#endif
