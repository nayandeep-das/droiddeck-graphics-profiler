
#include "diagnostic_engine.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

std::string toString(DiagnosticCategory category) {
    switch (category) {
        case DiagnosticCategory::FpsDegradation:
            return "FPS_DEGRADATION";

        case DiagnosticCategory::TimingAnomaly:
            return "TIMING_ANOMALY";

        case DiagnosticCategory::CombinedAnomaly:
            return "COMBINED_ANOMALY";
    }

    return "UNKNOWN";
}

std::vector<DiagnosticResult> generateDiagnostics(
    const std::vector<StutterEvent>& events
) {
    std::vector<DiagnosticResult> results;

    for (const auto& event : events) {

        // Ignore events without any detected condition.
        if (!event.fpsDegradation && !event.timingAnomaly) {
            continue;
        }

        DiagnosticResult result;

        result.timestamp = event.timestamp;
        result.severity = event.severity;
        result.displayedFps = event.displayedFps;

        // Preserve subsystem timing measurements.
        result.renderSceneMaxMs = event.renderSceneMaxMs;
        result.presentMaxMs = event.presentMaxMs;
        result.fenceWaitMaxMs = event.fenceWaitMaxMs;
        result.releaseMaxMs = event.releaseMaxMs;

        // Classify the detected event.
        if (event.fpsDegradation && event.timingAnomaly) {
            result.category = DiagnosticCategory::CombinedAnomaly;
        } else if (event.fpsDegradation) {
            result.category = DiagnosticCategory::FpsDegradation;
        } else {
            result.category = DiagnosticCategory::TimingAnomaly;
        }

        // Preserve the original detection reasons.
        result.evidence = event.reasons;

        // Add FPS and subsystem timing measurements.
        std::ostringstream evidence;
        evidence << std::fixed << std::setprecision(2);

        evidence << "Displayed FPS: "
                 << result.displayedFps;
        result.evidence.push_back(evidence.str());

        evidence.str("");
        evidence.clear();
        evidence << "Render Scene maximum: "
                 << result.renderSceneMaxMs << " ms";
        result.evidence.push_back(evidence.str());

        evidence.str("");
        evidence.clear();
        evidence << "Present maximum: "
                 << result.presentMaxMs << " ms";
        result.evidence.push_back(evidence.str());

        evidence.str("");
        evidence.clear();
        evidence << "Fence Wait maximum: "
                 << result.fenceWaitMaxMs << " ms";
        result.evidence.push_back(evidence.str());

        evidence.str("");
        evidence.clear();
        evidence << "Release maximum: "
                 << result.releaseMaxMs << " ms";
        result.evidence.push_back(evidence.str());

        // Generate an evidence-based interpretation.
        switch (result.category) {
            case DiagnosticCategory::FpsDegradation:
                result.interpretation =
                    "FPS degradation detected. None of the monitored "
                    "subsystem timing thresholds were exceeded in this "
                    "sample. The available telemetry does not identify "
                    "a specific subsystem bottleneck.";
                break;

            case DiagnosticCategory::TimingAnomaly:
                result.interpretation =
                    "One or more subsystem timing thresholds were "
                    "exceeded, but the displayed FPS did not cross "
                    "the configured degradation threshold. This is "
                    "a timing anomaly without detected FPS degradation.";
                break;

            case DiagnosticCategory::CombinedAnomaly:
                result.interpretation =
                    "FPS degradation and one or more subsystem timing "
                    "anomalies occurred in the same sample. This "
                    "indicates temporal correlation, but does not "
                    "establish that the timing anomaly caused the "
                    "FPS degradation.";
                break;
        }

        results.push_back(std::move(result));
    }

    return results;
}
