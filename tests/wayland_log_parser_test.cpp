
#include "wayland_log_parser.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <fstream>
#include <cstdio>
#include <vector>
#include <stdexcept>




namespace {
    void require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error("Test failed: " + message);
        }
    }

    bool nearlyEqual(double actual, double expected,
                     double tolerance = 0.001) {
        return std::abs(actual - expected) <= tolerance;
    }

    const WaylandLogSample* findSample(
        const std::vector<WaylandLogSample>& samples,
        const std::string& timestamp) {

        for (const auto& sample : samples) {
            if (sample.timestamp == timestamp) {
                return &sample;
            }
        }

        return nullptr;
    }
}

int main() {
    try {
        const auto samples = loadWaylandLog("data/wayland.log");

        // 1. Verify the complete stats dataset was loaded.
        require(samples.size() == 88,
                "Expected 88 samples, got " +
                std::to_string(samples.size()));

        // 2. Find and validate the known stuttering interval.
        const auto* known = findSample(samples, "18:25:15.670");

        require(known != nullptr,
                "Known interval 18:25:15.670 was not found");

        require(known->gpuReleaseWaits == 0,
                "GPU release wait count mismatch");

        require(known->hasStatsData,
                "Known interval should contain stats data");

        require(known->hasPerfData,
                "Known interval should contain perf data");

        require(known->windowsOpen == 1,
                "Unexpected open-window count");

        require(nearlyEqual(known->displayedFps, 4.10),
                "Unexpected FPS");

        require(nearlyEqual(known->renderSceneMaxMs, 5.78),
                "Unexpected render-scene maximum");

        require(nearlyEqual(known->releaseMaxMs, 35.58),
                "Unexpected release maximum");

        require(known->releaseFrameCount == 70,
                "Unexpected release frame count");

        require(known->releaseHeldFrames == 39,
                "Unexpected held-frame count");

        // 3. Verify capture-group mappings for another interval.
        const auto* mappingSample =
            findSample(samples, "18:23:15.645");

        require(mappingSample != nullptr,
                "Capture-mapping reference interval was not found");

        require(nearlyEqual(mappingSample->fenceWaitAvgMs, 2.12),
                "Fence-wait average mapping is incorrect");

        require(nearlyEqual(mappingSample->fenceWaitMaxMs, 5.64),
                "Fence-wait maximum mapping is incorrect");

        require(nearlyEqual(mappingSample->releaseAvgMs, 11.83),
                "Release average mapping is incorrect");

        require(nearlyEqual(mappingSample->releaseMaxMs, 32.78),
                "Release maximum mapping is incorrect");

        require(mappingSample->releaseFrameCount == 792,
                "Release frame-count mapping is incorrect");

        require(mappingSample->releaseHeldFrames == 740,
                "Held-frame mapping is incorrect");

        // 4. Verify window state after the game window closes.
        const auto* closedWindowSample =
            findSample(samples, "18:37:45.921");

        require(closedWindowSample != nullptr,
                "Window-closed reference interval was not found");

        require(closedWindowSample->windowsOpen == 0,
                "Expected zero open windows after game window closed");

        // 5. Verify GPU surface lifecycle tracking.

        // Surface should be active during gameplay.
        const auto* activeGameplaySample =
            findSample(samples, "18:25:15.670");

        require(activeGameplaySample != nullptr,
                "Active gameplay reference interval was not found");

        require(activeGameplaySample->surfaceActive,
                "GPU surface should be active during gameplay");

        // Surface should remain active immediately before detachment.
        const auto* beforeDetachSample =
            findSample(samples, "18:37:25.920");

        require(beforeDetachSample != nullptr,
                "Pre-detachment reference interval was not found");

        require(beforeDetachSample->surfaceActive,
                "GPU surface should be active before detachment");

        // Surface should become inactive after detachment.
        const auto* afterDetachSample =
            findSample(samples, "18:37:35.921");

        require(afterDetachSample != nullptr,
                "Post-detachment reference interval was not found");

        require(!afterDetachSample->surfaceActive,
                "GPU surface should be inactive after detachment");

        // Window count and GPU surface state are independent.
        require(afterDetachSample->windowsOpen == 1,
                "Expected one open window after surface detachment");

        // 6. Verify timestamps are in chronological order.
        for (std::size_t i = 1; i < samples.size(); ++i) {
            require(samples[i - 1].timestamp < samples[i].timestamp,
                    "Samples are not in strictly increasing timestamp order");
        }

        // 7. Reject malformed numeric values.
        {
                const std::string testFile = "data/malformed_wayland.log";

                {
                        std::ofstream output(testFile);

                        require(output.is_open(),
                                "Could not create malformed test log");

                        output
                                << "18:25:15.670 stats last 10 s: "
                                << "70 frames on screen (12.3.4 fps) | "
                                << "70 GPU frames from games, 1 windows open\n";
                }

                bool rejectedMalformedValue = false;

                try {
                        loadWaylandLog(testFile);
                } catch (const std::exception&) {
                        rejectedMalformedValue = true;
                }

                std::remove(testFile.c_str());

                require(
                        rejectedMalformedValue,
                        "Malformed FPS value should be rejected"
                );
        }

        
        // 8. Reject malformed stats records and report line number.
        {
            const std::string testFile = "data/malformed_wayland.log";

            {
                std::ofstream output(testFile);

                require(output.is_open(),
                        "Could not create malformed test log");

                output
                    << "18:25:16.670 stats last 10 s: "
                    << "70 frames on screen (oops fps) | "
                    << "70 GPU frames from games, 1 windows open\n";
            }

            bool rejectedMalformedRecord = false;
            std::string errorMessage;

            try {
                loadWaylandLog(testFile);
            } catch (const std::exception& error) {
                rejectedMalformedRecord = true;
                errorMessage = error.what();
            }

            std::remove(testFile.c_str());

            require(
                rejectedMalformedRecord,
                "Malformed stats record should be rejected"
            );

            require(
                errorMessage.find("line 1") != std::string::npos,
                "Error should report the malformed record's line number"
            );
        }

        // 9. Reject malformed perf records and report line number.
        {
                const std::string testFile = "data/malformed_wayland.log";

                {
                        std::ofstream output(testFile);
                        require(output.is_open(),
                                "Could not create malformed test log");

                        output
                                << "18:25:16.670 perf last 10 s: invalid performance data\n";
                }

                bool rejectedMalformedRecord = false;
                std::string errorMessage;

                try {
                        loadWaylandLog(testFile);
                }
                catch (const std::exception& error) {
                        rejectedMalformedRecord = true;
                        errorMessage = error.what();
                }

                std::remove(testFile.c_str());

                require(rejectedMalformedRecord,
                        "Malformed perf record should be rejected");

                require(errorMessage.find("line 1") != std::string::npos,
                        "Error should report the malformed record's line number");

                require(errorMessage.find("Malformed perf record") != std::string::npos,
                        "Error should identify the malformed perf record");
                }

        // 10. Handle incomplete telemetry records safely.
        {
                const std::string testFile = "data/incomplete_wayland.log";

                {
                        std::ofstream output(testFile);

                        require(output.is_open(),
                                "Could not create incomplete test log");

                        // Stats record without a matching perf record.
                        output
                                << "18:25:16.670 stats last 10 s: "
                                << "70 frames on screen (14.0 fps) | "
                                << "700 GPU frames from games, 1 windows open\n";

                        // Valid perf record without a matching stats record.
                        output
                                << "18:25:17.670 perf last 10 s: "
                                << "render_scene 1.00/2.00 ms "
                                << "acquire 1.00/2.00 ms "
                                << "present 1.00/2.00 ms (10) "
                                << "fence wait 1.00/2.00 ms "
                                << "(10, 0 GPU release waits) "
                                << "release 1.00/2.00 ms (10, 0 held)\n";
                }

                const auto samples = loadWaylandLog(testFile);

                std::remove(testFile.c_str());

                require(samples.size() == 1,
                        "Only the stats interval should be returned");

                require(samples[0].hasStatsData,
                        "Stats-only sample should retain stats data");

                require(!samples[0].hasPerfData,
                        "Stats-only sample should indicate missing perf data");

                require(samples[0].displayedFps == 14.0,
                        "Stats-only sample should preserve displayed FPS");
        }

        // 11. Lifecycle events should be processed chronologically,
        // even when they appear out of order in the input log.
        {
                const std::string testFile = "data/lifecycle_order_test.log";

                {
                        std::ofstream output(testFile);

                        require(output.is_open(),
                                "Could not create lifecycle test log");

                // Deliberately write the detach event first.
                output << "18:25:15.000 gpu screen surface gone\n";

                output
                        << "18:25:20.000 stats last 10 s: "
                        << "200 frames on screen (20.0 fps) | "
                        << "2000 GPU frames from games, 1 windows open\n";

                // Write the attach event afterwards.
                output << "18:25:05.000 gpu screen surface attached\n";

                output
                        << "18:25:10.000 stats last 10 s: "
                        << "300 frames on screen (30.0 fps) | "
                        << "3000 GPU frames from games, 1 windows open\n";
                }

                const auto samples = loadWaylandLog(testFile);

                std::remove(testFile.c_str());

                require(samples.size() == 2,
                        "Expected two lifecycle test samples");

                require(samples[0].timestamp == "18:25:10.000",
                        "Samples should be returned chronologically");

                require(samples[1].timestamp == "18:25:20.000",
                        "Second sample should have the later timestamp");

                require(samples[0].surfaceActive,
                        "Surface should be active after attachment");

                require(!samples[1].surfaceActive,
                        "Surface should be inactive after detachment");
        }

        // 12. Reject invalid floating-point values.
        {
                const std::vector<std::string> invalidValues = {
                        "12.3.4",
                        "nan",
                        "inf",
                        "-inf"
                };

                for (const auto& value : invalidValues) {
                        bool rejected = false;

                        try {
                                parseDoubleStrict(value);
                        }
                        catch (const std::exception&) {
                                rejected = true;
                        }

                        require(
                                rejected,
                                "Invalid numeric value should be rejected: " + value
                        );
                }

                require(
                        parseDoubleStrict("12.5") == 12.5,
                        "Valid floating-point values should be accepted"
                );
        }

        
        std::cout << "All Wayland log parser tests passed.\n";
        std::cout << "Validated samples: " << samples.size() << '\n';
        std::cout << "Surface lifecycle tests: PASSED\n";

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
