
#include "baseline_comparison.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace {

struct MetricsAccumulator {
    std::size_t count = 0;

    double totalFps = 0.0;
    double minimumFps = std::numeric_limits<double>::max();
    double maximumFps = 0.0;

    double totalRenderSceneMs = 0.0;
    double totalAcquireMs = 0.0;
    double totalPresentMs = 0.0;
    double totalFenceWaitMs = 0.0;
    double totalReleaseMs = 0.0;

    double maximumReleaseMs = 0.0;

    double totalHeldFrameRatio = 0.0;
    std::size_t heldRatioCount = 0;

    void addSample(const WaylandLogSample& sample) {
        ++count;

        totalFps += sample.displayedFps;

        minimumFps = std::min(minimumFps, sample.displayedFps);
        maximumFps = std::max(maximumFps, sample.displayedFps);

        totalRenderSceneMs += sample.renderSceneAvgMs;
        totalAcquireMs += sample.acquireAvgMs;
        totalPresentMs += sample.presentAvgMs;
        totalFenceWaitMs += sample.fenceWaitAvgMs;
        totalReleaseMs += sample.releaseAvgMs;

        maximumReleaseMs =
            std::max(maximumReleaseMs, sample.releaseMaxMs);

        if (sample.releaseFrameCount > 0) {
            totalHeldFrameRatio +=
                static_cast<double>(sample.releaseHeldFrames) /
                static_cast<double>(sample.releaseFrameCount);

            ++heldRatioCount;
        }
    }

    BaselineMetrics calculate() const {
        BaselineMetrics result;

        result.sampleCount = count;

        if (count == 0) {
            return result;
        }

        const double divisor = static_cast<double>(count);

        result.averageFps = totalFps / divisor;
        result.minimumFps = minimumFps;
        result.maximumFps = maximumFps;

        result.averageRenderSceneMs =
            totalRenderSceneMs / divisor;

        result.averageAcquireMs =
            totalAcquireMs / divisor;

        result.averagePresentMs =
            totalPresentMs / divisor;

        result.averageFenceWaitMs =
            totalFenceWaitMs / divisor;

        result.averageReleaseMs =
            totalReleaseMs / divisor;

        result.maximumReleaseMs = maximumReleaseMs;

        result.heldRatioSampleCount = heldRatioCount;

        if (heldRatioCount > 0) {
            result.averageHeldFrameRatio =
                totalHeldFrameRatio /
                static_cast<double>(heldRatioCount);
        }

        return result;
    }
};

MetricDifference calculateDifference(
    double criticalValue,
    double higherValue
) {
    MetricDifference result;

    result.criticalValue = criticalValue;
    result.higherValue = higherValue;

    result.absoluteDifference =
        criticalValue - higherValue;

    if (higherValue != 0.0) {
        result.percentageDifference =
            (result.absoluteDifference / higherValue) * 100.0;
    }

    return result;
}

} // namespace

BaselineComparison compareBaselines(
    const std::vector<WaylandLogSample>& samples
) {
    MetricsAccumulator criticalAccumulator;
    MetricsAccumulator higherAccumulator;

    for (const auto& sample : samples) {

        // Apply the same gameplay filters as main.cpp.
        if (!sample.hasStatsData ||
            !sample.hasPerfData ||
            sample.windowsOpen == 0 ||
            !sample.surfaceActive ||
            sample.gpuFramesFromGames == 0) {
            continue;
        }

        if (sample.displayedFps < 10.0) {
            criticalAccumulator.addSample(sample);
        }
        else if (sample.displayedFps >= 30.0) {
            higherAccumulator.addSample(sample);
        }
    }

    BaselineComparison result;

    result.critical = criticalAccumulator.calculate();
    result.higher = higherAccumulator.calculate();

    result.fps = calculateDifference(
        result.critical.averageFps,
        result.higher.averageFps
    );

    result.renderScene = calculateDifference(
        result.critical.averageRenderSceneMs,
        result.higher.averageRenderSceneMs
    );

    result.acquire = calculateDifference(
        result.critical.averageAcquireMs,
        result.higher.averageAcquireMs
    );

    result.present = calculateDifference(
        result.critical.averagePresentMs,
        result.higher.averagePresentMs
    );

    result.fenceWait = calculateDifference(
        result.critical.averageFenceWaitMs,
        result.higher.averageFenceWaitMs
    );

    result.release = calculateDifference(
        result.critical.averageReleaseMs,
        result.higher.averageReleaseMs
    );

    return result;
}
