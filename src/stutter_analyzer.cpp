
#include "stutter_analyzer.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

std::string toString(StutterSeverity severity) {
    switch (severity) {
        case StutterSeverity::Warning:
            return "WARNING";

        case StutterSeverity::High:
            return "HIGH";

        case StutterSeverity::Critical:
            return "CRITICAL";
    }

    return "UNKNOWN";
}

std::vector<StutterEvent> detectStutters(
    const std::vector<WaylandLogSample>& samples,
    const StutterThresholds& thresholds
) {
    if (thresholds.criticalFps >= thresholds.lowFps ||
        thresholds.criticalFps < 0.0 ||
        thresholds.lowFps < 0.0 ||
        thresholds.renderSceneMaxMs < 0.0 ||
        thresholds.presentMaxMs < 0.0 ||
        thresholds.fenceWaitMaxMs < 0.0 ||
        thresholds.releaseMaxMs < 0.0) {
        throw std::invalid_argument("Invalid stutter thresholds");
    }

    std::vector<StutterEvent> events;

    for (const auto& sample : samples) {
        if (!sample.hasStatsData || sample.windowsOpen == 0 || !sample.surfaceActive) {
            continue;
        }

        StutterEvent event;
        event.timestamp = sample.timestamp;
        event.displayedFps = sample.displayedFps;

        event.renderSceneMaxMs = sample.renderSceneMaxMs;
        event.presentMaxMs = sample.presentMaxMs;
        event.fenceWaitMaxMs = sample.fenceWaitMaxMs;
        event.releaseMaxMs = sample.releaseMaxMs;

        // Detect FPS-based degradation.
        if (sample.displayedFps < thresholds.criticalFps) {
            event.severity = StutterSeverity::Critical;
            event.fpsDegradation = true;
            event.reasons.push_back("Critically low displayed FPS");
        } else if (sample.displayedFps < thresholds.lowFps) {
            event.severity = StutterSeverity::High;
            event.fpsDegradation = true;
            event.reasons.push_back("Low displayed FPS");
        }

        // Only inspect timing metrics when perf data exists.
        if (sample.hasPerfData) {
            if (sample.renderSceneMaxMs > thresholds.renderSceneMaxMs) {
                event.timingAnomaly = true;
                event.reasons.push_back("High render-scene maximum");
            }

            if (sample.presentMaxMs > thresholds.presentMaxMs) {
                event.timingAnomaly = true;
                event.reasons.push_back("High present maximum");
            }

            if (sample.fenceWaitMaxMs > thresholds.fenceWaitMaxMs) {
                event.timingAnomaly = true;
                event.reasons.push_back("High fence-wait maximum");
            }

            if (sample.releaseMaxMs > thresholds.releaseMaxMs) {
                event.timingAnomaly = true;
                event.reasons.push_back("High release maximum");
            }

            // Timing anomalies are warnings unless FPS gives
            // the event a higher severity.
            if (!event.reasons.empty() &&
                event.severity == StutterSeverity::Warning) {
                // Keep warning severity.
            }
        }

        // Keep only intervals with at least one detected condition.
        if (!event.reasons.empty()) {
            events.push_back(std::move(event));
        }
    }

    // Rank events by severity, then by lower displayed FPS.
    std::sort(
        events.begin(),
        events.end(),
        [](const StutterEvent& a, const StutterEvent& b) {
            if (a.severity != b.severity) {
                return static_cast<int>(a.severity) >
                       static_cast<int>(b.severity);
            }

            return a.displayedFps < b.displayedFps;
        }
    );

    return events;
}
