
#ifndef DIAGNOSTIC_ENGINE_HPP
#define DIAGNOSTIC_ENGINE_HPP

#include "stutter_analyzer.hpp"

#include <string>
#include <vector>

enum class DiagnosticCategory {
    FpsDegradation,
    TimingAnomaly,
    CombinedAnomaly
};

struct DiagnosticResult {
    std::string timestamp;

    DiagnosticCategory category = DiagnosticCategory::TimingAnomaly;

    StutterSeverity severity = StutterSeverity::Warning;

    double displayedFps = 0.0;

    // Maximum subsystem timings from the original telemetry.
    double renderSceneMaxMs = 0.0;
    double presentMaxMs = 0.0;
    double fenceWaitMaxMs = 0.0;
    double releaseMaxMs = 0.0;

    std::vector<std::string> evidence;

    std::string interpretation;
};

std::vector<DiagnosticResult> generateDiagnostics(
    const std::vector<StutterEvent>& events
);

std::string toString(DiagnosticCategory category);

#endif
