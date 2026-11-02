
#include "recommendation_engine.hpp"

#include <cassert>
#include <iostream>
#include <set>

int main() {
    // Test 1: Empty hypothesis analysis
    {
        HypothesisAnalysis analysis;

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        assert(result.totalRecommendations == 0);
        assert(result.recommendations.empty());
        assert(result.highPriorityCount == 0);
        assert(result.mediumPriorityCount == 0);
        assert(result.lowPriorityCount == 0);

        std::cout << "[PASS] Empty hypothesis analysis\n";
    }

    // Test 2: Generate recommendations for all five hypotheses
    {
        HypothesisAnalysis analysis;

        const HypothesisType types[] = {
            HypothesisType::FrameProduction,
            HypothesisType::PresentationLifecycle,
            HypothesisType::ReleaseTiming,
            HypothesisType::RenderBottleneck,
            HypothesisType::Synchronization
        };

        for (const auto type : types) {
            DiagnosticHypothesis hypothesis;
            hypothesis.type = type;
            hypothesis.confidence = ConfidenceLevel::Low;
            hypothesis.evidenceScore = 0.0;

            analysis.hypotheses.push_back(hypothesis);
        }

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        assert(result.totalRecommendations == 5);
        assert(result.recommendations.size() == 5);

        std::cout << "[PASS] Five hypothesis recommendations generated\n";
    }

    // Test 3: Each hypothesis gets a distinct recommendation
    {
        HypothesisAnalysis analysis;

        const HypothesisType types[] = {
            HypothesisType::FrameProduction,
            HypothesisType::PresentationLifecycle,
            HypothesisType::ReleaseTiming,
            HypothesisType::RenderBottleneck,
            HypothesisType::Synchronization
        };

        for (const auto type : types) {
            DiagnosticHypothesis hypothesis;
            hypothesis.type = type;
            analysis.hypotheses.push_back(hypothesis);
        }

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        std::set<HypothesisType> generatedTypes;

        for (const auto& recommendation : result.recommendations) {
            generatedTypes.insert(recommendation.relatedHypothesis);
        }

        assert(generatedTypes.size() == 5);

        std::cout << "[PASS] All five hypothesis categories are covered\n";
    }

    // Test 4: Recommendation content is complete
    {
        HypothesisAnalysis analysis;

        DiagnosticHypothesis hypothesis;
        hypothesis.type = HypothesisType::FrameProduction;
        hypothesis.confidence = ConfidenceLevel::Low;
        hypothesis.evidenceScore = 33.33;

        analysis.hypotheses.push_back(hypothesis);

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        assert(result.totalRecommendations == 1);

        const auto& recommendation = result.recommendations.front();

        assert(!recommendation.title.empty());
        assert(!recommendation.rationale.empty());
        assert(!recommendation.investigationSteps.empty());
        assert(!recommendation.validationExperiments.empty());

        std::cout << "[PASS] Recommendation content is complete\n";
    }

    // Test 5: High-confidence evidence gets high priority
    {
        HypothesisAnalysis analysis;

        DiagnosticHypothesis hypothesis;
        hypothesis.type = HypothesisType::FrameProduction;
        hypothesis.confidence = ConfidenceLevel::High;
        hypothesis.evidenceScore = 85.0;

        analysis.hypotheses.push_back(hypothesis);

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        assert(result.highPriorityCount == 1);
        assert(result.mediumPriorityCount == 0);
        assert(result.lowPriorityCount == 0);

        assert(
            result.recommendations.front().priority ==
            RecommendationPriority::High
        );

        std::cout << "[PASS] High-confidence priority assignment\n";
    }

    // Test 6: Medium-confidence evidence gets medium priority
    {
        HypothesisAnalysis analysis;

        DiagnosticHypothesis hypothesis;
        hypothesis.type = HypothesisType::PresentationLifecycle;
        hypothesis.confidence = ConfidenceLevel::Medium;
        hypothesis.evidenceScore = 44.44;

        analysis.hypotheses.push_back(hypothesis);

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        assert(result.highPriorityCount == 0);
        assert(result.mediumPriorityCount == 1);
        assert(result.lowPriorityCount == 0);

        assert(
            result.recommendations.front().priority ==
            RecommendationPriority::Medium
        );

        std::cout << "[PASS] Medium-confidence priority assignment\n";
    }

    // Test 7: Low-confidence evidence gets low priority
    {
        HypothesisAnalysis analysis;

        DiagnosticHypothesis hypothesis;
        hypothesis.type = HypothesisType::ReleaseTiming;
        hypothesis.confidence = ConfidenceLevel::Low;
        hypothesis.evidenceScore = 0.0;

        analysis.hypotheses.push_back(hypothesis);

        const RecommendationAnalysis result =
            generateRecommendations(analysis);

        assert(result.highPriorityCount == 0);
        assert(result.mediumPriorityCount == 0);
        assert(result.lowPriorityCount == 1);

        assert(
            result.recommendations.front().priority ==
            RecommendationPriority::Low
        );

        std::cout << "[PASS] Low-confidence priority assignment\n";
    }

    // Test 8: Priority string conversion
    {
        assert(toString(RecommendationPriority::High) == "HIGH");
        assert(toString(RecommendationPriority::Medium) == "MEDIUM");
        assert(toString(RecommendationPriority::Low) == "LOW");

        std::cout << "[PASS] Priority string conversion\n";
    }

    std::cout << "\nAll 8 recommendation engine tests passed.\n";

    return 0;
}
