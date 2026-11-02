
#include "diagnostic_engine.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>


namespace {
    void require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error("Test failed: " + message);
        }
    }

    StutterEvent makeEvent(
        bool fpsDegradation,
        bool timingAnomaly
    ) {
        StutterEvent event;

        event.timestamp = "18:25:15.670";
        event.displayedFps = 8.5;
        event.severity = StutterSeverity::High;

        event.fpsDegradation = fpsDegradation;
        event.timingAnomaly = timingAnomaly;

        event.reasons = {"Test anomaly evidence"};

        return event;
    }
}

int main() {
    try {
        // 1. FPS degradation only.
        {
            const auto event = makeEvent(true, false);
            const auto results = generateDiagnostics({event});

            require(results.size() == 1,
                    "Expected one FPS degradation diagnostic");

            require(
                results[0].category ==
                    DiagnosticCategory::FpsDegradation,
                "FPS-only event was classified incorrectly"
            );
        }

        // 2. Timing anomaly only.
        {
            const auto event = makeEvent(false, true);
            const auto results = generateDiagnostics({event});

            require(results.size() == 1,
                    "Expected one timing anomaly diagnostic");

            require(
                results[0].category ==
                    DiagnosticCategory::TimingAnomaly,
                "Timing-only event was classified incorrectly"
            );
        }

        // 3. Combined anomaly.
        {
            const auto event = makeEvent(true, true);
            const auto results = generateDiagnostics({event});

            require(results.size() == 1,
                    "Expected one combined diagnostic");

            require(
                results[0].category ==
                    DiagnosticCategory::CombinedAnomaly,
                "Combined event was classified incorrectly"
            );
        }

        // 4. Verify original event information is preserved.
        {
            auto event = makeEvent(true, false);

            event.renderSceneMaxMs = 42.5;
            event.presentMaxMs = 12.3;
            event.fenceWaitMaxMs = 8.7;
            event.releaseMaxMs = 55.2;

            const auto results = generateDiagnostics({event});

            require(
                results[0].timestamp == event.timestamp,
                "Timestamp was not preserved"
            );

            require(
                results[0].displayedFps == event.displayedFps,
                "Displayed FPS was not preserved"
            );

            require(
                results[0].severity == event.severity,
                "Severity was not preserved"
            );

            require(
                results[0].renderSceneMaxMs == 42.5,
                "Render Scene timing was not preserved"
            );

            require(
                results[0].presentMaxMs == 12.3,
                "Present timing was not preserved"
            );

            require(
                results[0].fenceWaitMaxMs == 8.7,
                "Fence Wait timing was not preserved"
            );

            require(
                results[0].releaseMaxMs == 55.2,
                "Release timing was not preserved"
            );

            // Verify formatted subsystem measurements appear in evidence.
            const std::vector<std::string> expectedEvidence = {
                "Render Scene maximum: 42.50 ms",
                "Present maximum: 12.30 ms",
                "Fence Wait maximum: 8.70 ms",
                "Release maximum: 55.20 ms"
            };

            for (const auto& expected : expectedEvidence) {
                require(
                    std::find(
                        results[0].evidence.begin(),
                        results[0].evidence.end(),
                        expected
                    ) != results[0].evidence.end(),
                    "Formatted subsystem timing evidence is missing: " + expected
                );
            }

            require(
                !results[0].evidence.empty(),
                "Diagnostic evidence is missing"
            );

            require(
                results[0].evidence[0] == event.reasons[0],
                "Original detection reason was not preserved"
            );

            require(
                !results[0].interpretation.empty(),
                "Diagnostic interpretation is missing"
            );
        }

        // 5. Empty input.
        {
            const std::vector<StutterEvent> events;
            const auto results = generateDiagnostics(events);

            require(
                results.empty(),
                "Empty input should produce no diagnostics"
            );
        }

        // 6. Ignore events without any detected condition.
        {
            const auto event = makeEvent(false, false);
            const auto results = generateDiagnostics({event});

            require(
                results.empty(),
                "Event without detected conditions should be ignored"
            );
        }

        std::cout << "All 6 diagnostic engine tests passed.\n";

    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
