
#ifndef HYPOTHESIS_ENGINE_HPP
#define HYPOTHESIS_ENGINE_HPP


#include <cstddef>
#include "wayland_log_parser.h"

#include <string>
#include <vector>

enum class HypothesisType {
    FrameProduction,
    ReleaseTiming,
    PresentationLifecycle,
    RenderBottleneck,
    Synchronization
};

enum class ConfidenceLevel {
    Low,
    Medium,
    High
};

struct DiagnosticHypothesis {
    HypothesisType type = HypothesisType::FrameProduction;

    std::string title;
    std::string explanation;

    ConfidenceLevel confidence = ConfidenceLevel::Low;

    std::vector<std::string> supportingEvidence;
    std::vector<std::string> contradictoryEvidence;

    double evidenceScore = 0.0;
};

struct HypothesisAnalysis {
    std::vector<DiagnosticHypothesis> hypotheses;

    std::size_t analyzedSamples = 0;
    std::size_t criticalSamples = 0;
};

HypothesisAnalysis analyzeHypotheses(
    const std::vector<WaylandLogSample>& samples
);

std::string toString(HypothesisType type);

std::string toString(ConfidenceLevel confidence);

#endif
