
#ifndef RECOMMENDATION_ENGINE_HPP
#define RECOMMENDATION_ENGINE_HPP

#include "hypothesis_engine.hpp"

#include <string>
#include <vector>

enum class RecommendationPriority {
    Low,
    Medium,
    High
};

struct DiagnosticRecommendation {
    HypothesisType relatedHypothesis =
        HypothesisType::FrameProduction;

    RecommendationPriority priority =
        RecommendationPriority::Low;

    std::string title;
    std::string rationale;

    std::vector<std::string> investigationSteps;
    std::vector<std::string> validationExperiments;
};

struct RecommendationAnalysis {
    std::vector<DiagnosticRecommendation> recommendations;

    std::size_t totalRecommendations = 0;
    std::size_t highPriorityCount = 0;
    std::size_t mediumPriorityCount = 0;
    std::size_t lowPriorityCount = 0;
};

RecommendationAnalysis generateRecommendations(
    const HypothesisAnalysis& analysis
);

std::string toString(RecommendationPriority priority);

#endif
