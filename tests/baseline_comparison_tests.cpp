
#include "baseline_comparison.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

WaylandLogSample createSample(
    double fps,
    double render,
    double acquire,
    double present,
    double fence,
    double release,
    double releaseMax,
    long long held,
    long long frameCount
) {
    WaylandLogSample sample;

    sample.hasStatsData = true;
    sample.hasPerfData = true;
    sample.surfaceActive = true;

    sample.windowsOpen = 1;
    sample.gpuFramesFromGames = 100;

    sample.displayedFps = fps;

    sample.renderSceneAvgMs = render;
    sample.acquireAvgMs = acquire;
    sample.presentAvgMs = present;
    sample.fenceWaitAvgMs = fence;

    sample.releaseAvgMs = release;
    sample.releaseMaxMs = releaseMax;

    sample.releaseHeldFrames = held;
    sample.releaseFrameCount = frameCount;

    return sample;
}

int main() {
    try {
        // Test 1: Critical and higher FPS grouping.
        {
            std::vector<WaylandLogSample> samples;

            samples.push_back(
                createSample(5.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 20.0, 40, 100)
            );

            samples.push_back(
                createSample(8.0, 8.0, 2.0, 4.0, 5.0,
                             6.0, 30.0, 60, 100)
            );

            samples.push_back(
                createSample(35.0, 4.0, 1.0, 2.0, 3.0,
                             10.0, 40.0, 80, 100)
            );

            samples.push_back(
                createSample(40.0, 6.0, 3.0, 4.0, 5.0,
                             14.0, 50.0, 60, 100)
            );

            const auto result = compareBaselines(samples);

            require(result.critical.sampleCount == 2,
                    "Expected two critical samples");

            require(result.higher.sampleCount == 2,
                    "Expected two higher-FPS samples");

            require(std::abs(result.critical.averageFps - 6.5) < 0.001,
                    "Incorrect critical average FPS");

            require(std::abs(result.higher.averageFps - 37.5) < 0.001,
                    "Incorrect higher average FPS");

            require(std::abs(result.critical.minimumFps - 5.0) < 0.001,
                    "Incorrect critical minimum FPS");

            require(std::abs(result.higher.maximumFps - 40.0) < 0.001,
                    "Incorrect higher maximum FPS");
        }

        // Test 2: Subsystem timing averages.
        {
            std::vector<WaylandLogSample> samples;

            samples.push_back(
                createSample(5.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 20.0, 40, 100)
            );

            samples.push_back(
                createSample(35.0, 10.0, 3.0, 4.0, 5.0,
                             12.0, 40.0, 80, 100)
            );

            const auto result = compareBaselines(samples);

            require(
                std::abs(result.critical.averageRenderSceneMs - 6.0) < 0.001,
                "Incorrect critical Render Scene average"
            );

            require(
                std::abs(result.higher.averageRenderSceneMs - 10.0) < 0.001,
                "Incorrect higher Render Scene average"
            );

            require(
                std::abs(result.critical.averageReleaseMs - 4.0) < 0.001,
                "Incorrect critical Release average"
            );

            require(
                std::abs(result.higher.averageReleaseMs - 12.0) < 0.001,
                "Incorrect higher Release average"
            );
        }

        // Test 3: Difference calculations.
        {
            std::vector<WaylandLogSample> samples;

            samples.push_back(
                createSample(5.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 20.0, 40, 100)
            );

            samples.push_back(
                createSample(35.0, 10.0, 3.0, 4.0, 5.0,
                             12.0, 40.0, 80, 100)
            );

            const auto result = compareBaselines(samples);

            require(
                std::abs(result.renderScene.absoluteDifference + 4.0) < 0.001,
                "Incorrect absolute Render Scene difference"
            );

            require(
                result.renderScene.percentageDifference.has_value(),
                "Render Scene percentage difference should exist"
            );

            require(
                std::abs(*result.renderScene.percentageDifference + 40.0) < 0.001,
                "Incorrect Render Scene percentage difference"
            );
        }

        // Test 4: Maximum Release latency.
        {
            std::vector<WaylandLogSample> samples;

            samples.push_back(
                createSample(5.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 20.0, 40, 100)
            );

            samples.push_back(
                createSample(8.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 55.0, 50, 100)
            );

            const auto result = compareBaselines(samples);

            require(
                std::abs(result.critical.maximumReleaseMs - 55.0) < 0.001,
                "Incorrect maximum Release latency"
            );
        }

        // Test 5: Gameplay filtering.
        {
            std::vector<WaylandLogSample> samples;

            auto valid = createSample(
                5.0, 6.0, 1.0, 2.0, 3.0, 4.0, 20.0, 40, 100
            );

            samples.push_back(valid);

            auto missingPerf = valid;
            missingPerf.hasPerfData = false;
            samples.push_back(missingPerf);

            auto inactiveSurface = valid;
            inactiveSurface.surfaceActive = false;
            samples.push_back(inactiveSurface);

            auto noWindow = valid;
            noWindow.windowsOpen = 0;
            samples.push_back(noWindow);

            auto noGpuFrames = valid;
            noGpuFrames.gpuFramesFromGames = 0;
            samples.push_back(noGpuFrames);

            auto noStats = valid;
            noStats.hasStatsData = false;
            samples.push_back(noStats);

            const auto result = compareBaselines(samples);

            require(
                result.critical.sampleCount == 1,
                "Gameplay filters did not exclude invalid samples"
            );
        }

        // Test 6: Empty input.
        {
            const std::vector<WaylandLogSample> samples;

            const auto result = compareBaselines(samples);

            require(result.critical.sampleCount == 0,
                    "Empty input should produce zero critical samples");

            require(result.higher.sampleCount == 0,
                    "Empty input should produce zero higher samples");
        }

        // Test 7: Held-frame ratio.
        {
            std::vector<WaylandLogSample> samples;

            samples.push_back(
                createSample(5.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 20.0, 50, 100)
            );

            samples.push_back(
                createSample(8.0, 6.0, 1.0, 2.0, 3.0,
                             4.0, 20.0, 75, 100)
            );

            const auto result = compareBaselines(samples);

            require(
                std::abs(result.critical.averageHeldFrameRatio - 0.625) < 0.001,
                "Incorrect average held-frame ratio"
            );

            require(
                result.critical.heldRatioSampleCount == 2,
                "Incorrect held-frame ratio sample count"
            );
        }

        std::cout << "All baseline comparison tests passed.\n";
        return 0;

    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
