
#include "stutter_analyzer.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Test failed: " + message);
    }
}

WaylandLogSample makeSample(
    std::string timestamp,
    double fps,
    bool hasPerf = true
) {
    WaylandLogSample sample;

    sample.timestamp = std::move(timestamp);
    sample.hasStatsData = true;
    sample.hasPerfData = hasPerf;
    sample.displayedFps = fps;
    sample.windowsOpen = 1;
    sample.surfaceActive = true;

    return sample;
}

int main() {
    try {
        // Test 1: Normal interval should not be flagged.
        {
            auto sample = makeSample("10:00:00.000", 40.0);

            const auto events = detectStutters({sample});

            require(events.empty(),
                    "Normal interval was incorrectly flagged");
        }

        // Test 2: Critically low FPS should be detected.
        {
            auto sample = makeSample("10:00:10.000", 4.1);

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "Critical FPS interval was not detected");

            require(events[0].severity == StutterSeverity::Critical,
                    "Critical FPS severity is incorrect");
        }

        // Test 3: Low FPS should produce High severity.
        {
            auto sample = makeSample("10:00:20.000", 12.0);

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "Low FPS interval was not detected");

            require(events[0].severity == StutterSeverity::High,
                    "Low FPS severity is incorrect");
        }

        // Test 4: High timing with normal FPS should be a warning.
        {
            auto sample = makeSample("10:00:30.000", 40.0);
            sample.renderSceneMaxMs = 75.0;

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "High render timing was not detected");

            require(events[0].severity == StutterSeverity::Warning,
                    "Timing anomaly severity is incorrect");

            require(!events[0].reasons.empty(),
                    "Timing anomaly reason is missing");
        }

        // Test 5: Missing perf data should not create timing anomalies.
        {
            auto sample = makeSample(
                "10:00:40.000", 12.0, false
            );

            sample.renderSceneMaxMs = 200.0;

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "Low FPS should still be detected without perf data");

            require(events[0].reasons.size() == 1,
                    "Missing perf data created a false timing anomaly");
        }

        // Test 6: Normal FPS and normal timings.
        {
            auto sample = makeSample("10:00:45.000", 40.0);

            const auto events = detectStutters({sample});

            require(events.empty(),
                    "Normal performance generated an event");
        }

        // Test 7: FPS degradation only.
        {
            auto sample = makeSample("10:00:50.000", 12.0);

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "FPS degradation was not detected");

            require(events[0].fpsDegradation,
                    "FPS degradation flag was not set");

            require(!events[0].timingAnomaly,
                    "Unexpected timing anomaly flag");
        }

        // Test 8: Timing anomaly only.
        {
            auto sample = makeSample("10:00:55.000", 40.0);
            sample.releaseMaxMs = 75.0;

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "Timing anomaly was not detected");

            require(!events[0].fpsDegradation,
                    "Unexpected FPS degradation flag");

            require(events[0].timingAnomaly,
                    "Timing anomaly flag was not set");
        }

        // Test 9: Both FPS degradation and timing anomaly.
        {
            auto sample = makeSample("10:01:05.000", 4.1);
            sample.releaseMaxMs = 75.0;

            const auto events = detectStutters({sample});

            require(events.size() == 1,
                    "Combined anomaly was not detected");

            require(events[0].fpsDegradation,
                    "Combined event missed FPS degradation");

            require(events[0].timingAnomaly,
                    "Combined event missed timing anomaly");

            require(events[0].severity == StutterSeverity::Critical,
                    "Combined event severity is incorrect");
        }

        // Test 10: Events should be ranked by severity.
        {
            auto critical = makeSample("10:01:00.000", 4.1);
            auto high = makeSample("10:01:10.000", 12.0);

            auto warning = makeSample("10:01:20.000", 40.0);
            warning.presentMaxMs = 80.0;

            const auto events = detectStutters({
                warning, high, critical
            });

            require(events.size() == 3,
                    "Expected three detected events");

            require(events[0].severity == StutterSeverity::Critical,
                    "Critical event should rank first");

            require(events[1].severity == StutterSeverity::High,
                    "High event should rank second");

            require(events[2].severity == StutterSeverity::Warning,
                    "Warning event should rank third");
        }

        // Test 11: Samples with no open windows should be ignored.
        {
            auto sample = makeSample("18:37:45.921", 0.0);

            sample.windowsOpen = 0;
            sample.gpuFramesFromGames = 446;

            const auto events = detectStutters({sample});

            require(events.empty(),
                    "Window lifecycle sample was incorrectly detected as a stutter");
        }

        // Test 12: Samples with inactive GPU surfaces should be ignored.
        {
            auto sample = makeSample("18:37:35.921", 1.0);

            sample.windowsOpen = 1;
            sample.surfaceActive = false;

            const auto events = detectStutters({sample});

            require(events.empty(),
                    "Inactive GPU surface sample was incorrectly detected as a stutter");
        }

        // Test 13: Invalid thresholds should throw an exception.
        {
            StutterThresholds thresholds;
            thresholds.criticalFps = 20.0;
            thresholds.lowFps = 15.0;

            bool exceptionThrown = false;

            try {
                detectStutters({}, thresholds);
            } catch (const std::invalid_argument&) {
                exceptionThrown = true;
            }

            require(exceptionThrown,
                    "Invalid thresholds were not rejected");
        }

        // Test 14: Severity names should be readable.
        {
            require(
                toString(StutterSeverity::Critical) == "CRITICAL",
                "Critical severity string is incorrect"
            );

            require(
                toString(StutterSeverity::High) == "HIGH",
                "High severity string is incorrect"
            );

            require(
                toString(StutterSeverity::Warning) == "WARNING",
                "Warning severity string is incorrect"
            );
        }

        std::cout << "All stutter analyzer tests passed.\n";
        std::cout << "Validated 14 test cases.\n";
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
