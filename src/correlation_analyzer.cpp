
#include "correlation_analyzer.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace {

struct GroupAccumulator {
    std::size_t count = 0;

    double totalFps = 0.0;
    double totalGpuFrames = 0.0;
    double totalFramesOnScreen = 0.0;
    double totalHeldRatio = 0.0;
    double totalGpuReleaseWaits = 0.0;

    std::size_t heldRatioSamples = 0;

    void add(const WaylandLogSample& sample) {
        ++count;

        totalFps += sample.displayedFps;
        totalGpuFrames +=
            static_cast<double>(sample.gpuFramesFromGames);
        totalFramesOnScreen +=
            static_cast<double>(sample.framesOnScreen);
        totalGpuReleaseWaits +=
            static_cast<double>(sample.gpuReleaseWaits);

        if (sample.releaseFrameCount > 0) {
            const double heldRatio =
                static_cast<double>(sample.releaseHeldFrames) /
                static_cast<double>(sample.releaseFrameCount);

            totalHeldRatio += heldRatio;
            ++heldRatioSamples;
        }
    }
};

struct CorrelationAccumulator {
    std::vector<std::pair<double, double>> values;

    void add(double x, double y) {
        values.emplace_back(x, y);
    }

    CorrelationResult calculate() const {
        CorrelationResult result;
        result.sampleCount = values.size();

        if (values.size() < 2) {
            return result;
        }

        double sumX = 0.0;
        double sumY = 0.0;

        for (const auto& value : values) {
            sumX += value.first;
            sumY += value.second;
        }

        const double meanX =
            sumX / static_cast<double>(values.size());

        const double meanY =
            sumY / static_cast<double>(values.size());

        double covariance = 0.0;
        double varianceX = 0.0;
        double varianceY = 0.0;

        for (const auto& value : values) {
            const double differenceX = value.first - meanX;
            const double differenceY = value.second - meanY;

            covariance += differenceX * differenceY;
            varianceX += differenceX * differenceX;
            varianceY += differenceY * differenceY;
        }

        // Correlation is undefined if either variable is constant.
        if (varianceX == 0.0 || varianceY == 0.0) {
            return result;
        }

        double coefficient =
            covariance / std::sqrt(varianceX * varianceY);

        // Protect against minor floating-point rounding outside [-1, 1].
        coefficient = std::max(-1.0, std::min(1.0, coefficient));

        result.coefficient = coefficient;

        return result;
    }
};

void copyGroupMetrics(
    const GroupAccumulator& group,
    double& averageFps,
    double& averageGpuFrames,
    double& averageFramesOnScreen,
    double& averageHeldRatio,
    double& averageGpuReleaseWaits
) {
    if (group.count == 0) {
        return;
    }

    const double count = static_cast<double>(group.count);

    averageFps = group.totalFps / count;
    averageGpuFrames = group.totalGpuFrames / count;
    averageFramesOnScreen = group.totalFramesOnScreen / count;
    averageGpuReleaseWaits = group.totalGpuReleaseWaits / count;

    if (group.heldRatioSamples > 0) {
        averageHeldRatio =
            group.totalHeldRatio /
            static_cast<double>(group.heldRatioSamples);
    }
}

} // namespace

CorrelationAnalysis analyzeCorrelations(
    const std::vector<WaylandLogSample>& samples
) {
    CorrelationAnalysis analysis;

    GroupAccumulator critical;
    GroupAccumulator higher;

    CorrelationAccumulator fpsVsGpuFrames;
    CorrelationAccumulator fpsVsFramesOnScreen;
    CorrelationAccumulator fpsVsHeldFrameRatio;
    CorrelationAccumulator fpsVsGpuReleaseWaits;

    for (const auto& sample : samples) {
        // Use the same gameplay eligibility criteria as baseline comparison.
        if (!sample.hasStatsData ||
            !sample.hasPerfData ||
            sample.windowsOpen == 0 ||
            !sample.surfaceActive ||
            sample.gpuFramesFromGames == 0) {
            continue;
        }

        ++analysis.gameplaySampleCount;

        // Critical and higher-FPS groups only.
        if (sample.displayedFps < 10.0) {
            critical.add(sample);
        } else if (sample.displayedFps >= 30.0) {
            higher.add(sample);
        }

        // FPS against frame-production metrics.
        fpsVsGpuFrames.add(
            sample.displayedFps,
            static_cast<double>(sample.gpuFramesFromGames)
        );

        fpsVsFramesOnScreen.add(
            sample.displayedFps,
            static_cast<double>(sample.framesOnScreen)
        );

        fpsVsGpuReleaseWaits.add(
            sample.displayedFps,
            static_cast<double>(sample.gpuReleaseWaits)
        );

        // Only calculate held ratio when its denominator is valid.
        if (sample.releaseFrameCount > 0) {
            const double heldRatio =
                static_cast<double>(sample.releaseHeldFrames) /
                static_cast<double>(sample.releaseFrameCount);

            fpsVsHeldFrameRatio.add(
                sample.displayedFps,
                heldRatio
            );
        }
    }

    analysis.criticalSampleCount = critical.count;
    analysis.higherFpsSampleCount = higher.count;

    analysis.criticalHeldRatioSamples = critical.heldRatioSamples;
    analysis.higherHeldRatioSamples = higher.heldRatioSamples;

    copyGroupMetrics(
        critical,
        analysis.criticalAverageFps,
        analysis.criticalAverageGpuFrames,
        analysis.criticalAverageFramesOnScreen,
        analysis.criticalAverageHeldRatio,
        analysis.criticalAverageGpuReleaseWaits
    );

    copyGroupMetrics(
        higher,
        analysis.higherAverageFps,
        analysis.higherAverageGpuFrames,
        analysis.higherAverageFramesOnScreen,
        analysis.higherAverageHeldRatio,
        analysis.higherAverageGpuReleaseWaits
    );

    analysis.fpsVsGpuFrames = fpsVsGpuFrames.calculate();
    analysis.fpsVsFramesOnScreen = fpsVsFramesOnScreen.calculate();
    analysis.fpsVsHeldFrameRatio = fpsVsHeldFrameRatio.calculate();
    analysis.fpsVsGpuReleaseWaits = fpsVsGpuReleaseWaits.calculate();

    return analysis;
}
