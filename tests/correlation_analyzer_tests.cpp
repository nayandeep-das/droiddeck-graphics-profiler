
#include "correlation_analyzer.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

WaylandLogSample makeSample(
    double fps,
    long long gpuFrames,
    long long framesOnScreen,
    long long releaseFrameCount,
    long long releaseHeldFrames
) {
    WaylandLogSample sample;

    sample.hasStatsData = true;
    sample.hasPerfData = true;
    sample.surfaceActive = true;
    sample.windowsOpen = 1;

    sample.displayedFps = fps;
    sample.gpuFramesFromGames = gpuFrames;
    sample.framesOnScreen = framesOnScreen;

    sample.releaseFrameCount = releaseFrameCount;
    sample.releaseHeldFrames = releaseHeldFrames;

    return sample;
}

void testPositiveCorrelation() {
    std::vector<WaylandLogSample> samples = {
        makeSample(10.0, 100, 100, 10, 2),
        makeSample(20.0, 200, 200, 10, 4),
        makeSample(30.0, 300, 300, 10, 6)
    };

    const auto result = analyzeCorrelations(samples);

    assert(result.fpsVsGpuFrames.coefficient.has_value());
    assert(std::abs(
        *result.fpsVsGpuFrames.coefficient - 1.0
    ) < 0.0001);

    assert(result.fpsVsFramesOnScreen.coefficient.has_value());
    assert(std::abs(
        *result.fpsVsFramesOnScreen.coefficient - 1.0
    ) < 0.0001);
}

void testNegativeCorrelation() {
    std::vector<WaylandLogSample> samples = {
        makeSample(10.0, 300, 300, 10, 2),
        makeSample(20.0, 200, 200, 10, 4),
        makeSample(30.0, 100, 100, 10, 6)
    };

    const auto result = analyzeCorrelations(samples);

    assert(result.fpsVsGpuFrames.coefficient.has_value());
    assert(std::abs(
        *result.fpsVsGpuFrames.coefficient + 1.0
    ) < 0.0001);
}

void testConstantValuesProduceUndefinedCorrelation() {
    std::vector<WaylandLogSample> samples = {
        makeSample(10.0, 100, 100, 10, 2),
        makeSample(20.0, 100, 200, 10, 4),
        makeSample(30.0, 100, 300, 10, 6)
    };

    const auto result = analyzeCorrelations(samples);

    assert(!result.fpsVsGpuFrames.coefficient.has_value());
}

void testInsufficientSamples() {
    std::vector<WaylandLogSample> samples = {
        makeSample(10.0, 100, 100, 10, 2)
    };

    const auto result = analyzeCorrelations(samples);

    assert(result.fpsVsGpuFrames.sampleCount == 1);
    assert(!result.fpsVsGpuFrames.coefficient.has_value());
}

void testGameplayFiltering() {
    auto valid = makeSample(5.0, 100, 100, 10, 2);

    auto missingStats = makeSample(5.0, 100, 100, 10, 2);
    missingStats.hasStatsData = false;

    auto missingPerf = makeSample(5.0, 100, 100, 10, 2);
    missingPerf.hasPerfData = false;

    auto inactiveSurface = makeSample(5.0, 100, 100, 10, 2);
    inactiveSurface.surfaceActive = false;

    auto noWindows = makeSample(5.0, 100, 100, 10, 2);
    noWindows.windowsOpen = 0;

    auto noGpuFrames = makeSample(5.0, 0, 100, 10, 2);

    std::vector<WaylandLogSample> samples = {
        valid,
        missingStats,
        missingPerf,
        inactiveSurface,
        noWindows,
        noGpuFrames
    };

    const auto result = analyzeCorrelations(samples);

    assert(result.gameplaySampleCount == 1);
    assert(result.criticalSampleCount == 1);
    assert(result.higherFpsSampleCount == 0);
}

void testMissingHeldRatio() {
    std::vector<WaylandLogSample> samples = {
        makeSample(10.0, 100, 100, 10, 2),
        makeSample(20.0, 200, 200, 0, 0),
        makeSample(30.0, 300, 300, 10, 6)
    };

    const auto result = analyzeCorrelations(samples);

    assert(result.fpsVsHeldFrameRatio.sampleCount == 2);
    assert(result.fpsVsHeldFrameRatio.coefficient.has_value());
}

void testCriticalAndHigherGroups() {
    std::vector<WaylandLogSample> samples = {
        makeSample(4.0, 100, 40, 10, 2),
        makeSample(8.0, 200, 80, 10, 4),
        makeSample(35.0, 300, 350, 10, 6),
        makeSample(40.0, 400, 400, 10, 8)
    };

    const auto result = analyzeCorrelations(samples);

    assert(result.criticalSampleCount == 2);
    assert(result.higherFpsSampleCount == 2);

    assert(std::abs(result.criticalAverageFps - 6.0) < 0.0001);
    assert(std::abs(result.higherAverageFps - 37.5) < 0.0001);

    assert(std::abs(result.criticalAverageGpuFrames - 150.0) < 0.0001);
    assert(std::abs(result.higherAverageGpuFrames - 350.0) < 0.0001);

    assert(std::abs(result.criticalAverageHeldRatio - 0.3) < 0.0001);
    assert(std::abs(result.higherAverageHeldRatio - 0.7) < 0.0001);
}

} // namespace

int main() {
    testPositiveCorrelation();
    testNegativeCorrelation();
    testConstantValuesProduceUndefinedCorrelation();
    testInsufficientSamples();
    testGameplayFiltering();
    testMissingHeldRatio();
    testCriticalAndHigherGroups();

    std::cout << "All Correlation Analyzer tests passed.\n";

    return 0;
}
