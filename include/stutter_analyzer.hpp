
#ifndef STUTTER_ANALYZER_HPP
#define STUTTER_ANALYZER_HPP

#include "wayland_log_parser.h"

#include <string>
#include <vector>

enum class StutterSeverity {
    Warning,
    High,
    Critical
};

struct StutterEvent {
    std::string timestamp;

    double displayedFps = 0.0;

    StutterSeverity severity = StutterSeverity::Warning;

    bool fpsDegradation = false;
    bool timingAnomaly = false;

    std::vector<std::string> reasons;

    double renderSceneMaxMs = 0.0;
    double presentMaxMs = 0.0;
    double fenceWaitMaxMs = 0.0;
    double releaseMaxMs = 0.0;
};

struct StutterThresholds {
    double lowFps = 15.0;
    double criticalFps = 10.0;

    double renderSceneMaxMs = 50.0;
    double presentMaxMs = 50.0;
    double fenceWaitMaxMs = 50.0;
    double releaseMaxMs = 50.0;
};

std::vector<StutterEvent> detectStutters(
    const std::vector<WaylandLogSample>& samples,
    const StutterThresholds& thresholds = {}
);

std::string toString(StutterSeverity severity);

#endif
