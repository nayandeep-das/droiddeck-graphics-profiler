
#include "recommendation_engine.hpp"

#include <sstream>
#include <utility>

namespace {

RecommendationPriority determinePriority(
    const DiagnosticHypothesis& hypothesis
) {
    if (hypothesis.confidence == ConfidenceLevel::High &&
        hypothesis.evidenceScore >= 75.0) {
        return RecommendationPriority::High;
    }

    if (hypothesis.confidence == ConfidenceLevel::Medium &&
        hypothesis.evidenceScore >= 40.0) {
        return RecommendationPriority::Medium;
    }

    return RecommendationPriority::Low;
}

DiagnosticRecommendation buildRecommendation(
    const DiagnosticHypothesis& hypothesis
) {
    DiagnosticRecommendation recommendation;

    recommendation.relatedHypothesis = hypothesis.type;
    recommendation.priority = determinePriority(hypothesis);

    switch (hypothesis.type) {

        case HypothesisType::FrameProduction:
            recommendation.title =
                "Investigate Frame Production";

            recommendation.rationale =
                "Low FPS coincides with reduced GPU frame production. "
                "However, the existing measurements may be coupled to "
                "displayed FPS, so independent measurements are needed.";

            recommendation.investigationSteps = {
                "Collect independent application-side frame production data.",
                "Measure CPU frame submission and GPU frame completion rates.",
                "Compare frame production against displayed FPS over identical time windows.",
                "Check whether frame production drops before or after FPS degradation."
            };

            recommendation.validationExperiments = {
                "Capture application-side frame timestamps alongside Wayland statistics.",
                "Compare CPU and GPU frame rates during normal and low-FPS intervals.",
                "Check whether reduced frame production consistently precedes FPS degradation."
            };
            break;

        case HypothesisType::PresentationLifecycle:
            recommendation.title =
                "Investigate Presentation Lifecycle";

            recommendation.rationale =
                "Some critical FPS samples occur near presentation surface "
                "transitions. Other critical samples occur outside the "
                "observed transition window, so lifecycle activity alone "
                "does not explain every degradation.";

            recommendation.investigationSteps = {
                "Record exact GPU surface attach and detach timestamps.",
                "Correlate lifecycle events with FPS and window-count changes.",
                "Distinguish an existing application window from an active presentation surface.",
                "Inspect whether presentation resumes normally after a surface transition."
            };

            recommendation.validationExperiments = {
                "Capture repeated gameplay sessions with lifecycle events logged.",
                "Compare FPS immediately before and after surface transitions.",
                "Check whether similar FPS drops occur without lifecycle transitions."
            };
            break;

        case HypothesisType::ReleaseTiming:
            recommendation.title =
                "Investigate Buffer Release Behaviour";

            recommendation.rationale =
                "Current critical intervals do not show higher average "
                "release latency than higher-FPS intervals. Further "
                "measurement is needed before treating buffer release "
                "as a bottleneck.";

            recommendation.investigationSteps = {
                "Collect higher-resolution buffer release timing data.",
                "Track held-frame counts and release-frame counts over time.",
                "Inspect buffer release behaviour during individual stutter events.",
                "Check whether delayed releases occur repeatedly or only as isolated outliers."
            };

            recommendation.validationExperiments = {
                "Compare buffer release latency during repeated normal and low-FPS intervals.",
                "Check whether release delays consistently overlap with frame-time spikes.",
                "Repeat the analysis using individual events instead of aggregated ten-second samples."
            };
            break;

        case HypothesisType::RenderBottleneck:
            recommendation.title =
                "Investigate Rendering Workload";

            recommendation.rationale =
                "The current aggregate render timings do not show a clear "
                "increase during critical FPS intervals. More granular "
                "GPU workload measurements are required.";

            recommendation.investigationSteps = {
                "Collect per-frame GPU execution and render-stage timings.",
                "Inspect render workload during individual FPS degradation events.",
                "Separate average render timings from isolated maximum-latency spikes.",
                "Compare rendering behaviour across repeated gameplay sessions."
            };

            recommendation.validationExperiments = {
                "Capture per-frame GPU timings during normal and degraded gameplay.",
                "Check whether GPU execution time increases before FPS drops.",
                "Repeat the test under the same resolution and graphics configuration."
            };
            break;

        case HypothesisType::Synchronization:
            recommendation.title =
                "Investigate GPU Synchronization";

            recommendation.rationale =
                "Acquire and fence-wait averages are not higher in the "
                "critical group. Synchronization is therefore not currently "
                "supported as the primary explanation.";

            recommendation.investigationSteps = {
                "Collect per-frame acquire and fence-wait measurements.",
                "Inspect synchronization events around individual stutters.",
                "Check for unusually long waits that may be hidden by ten-second averages.",
                "Compare synchronization behaviour across normal and degraded intervals."
            };

            recommendation.validationExperiments = {
                "Capture high-resolution acquire and fence-wait traces during gameplay.",
                "Check whether synchronization delays overlap with frame-time spikes.",
                "Compare repeated low-FPS events against normal gameplay."
            };
            break;
    }

    return recommendation;
}

} // namespace

std::string toString(RecommendationPriority priority) {
    switch (priority) {
        case RecommendationPriority::High:
            return "HIGH";

        case RecommendationPriority::Medium:
            return "MEDIUM";

        case RecommendationPriority::Low:
            return "LOW";
    }

    return "UNKNOWN";
}

RecommendationAnalysis generateRecommendations(
    const HypothesisAnalysis& analysis
) {
    RecommendationAnalysis result;

    for (const auto& hypothesis : analysis.hypotheses) {
        DiagnosticRecommendation recommendation =
            buildRecommendation(hypothesis);

        switch (recommendation.priority) {
            case RecommendationPriority::High:
                ++result.highPriorityCount;
                break;

            case RecommendationPriority::Medium:
                ++result.mediumPriorityCount;
                break;

            case RecommendationPriority::Low:
                ++result.lowPriorityCount;
                break;
        }

        result.recommendations.push_back(
            std::move(recommendation)
        );
    }

    result.totalRecommendations =
        result.recommendations.size();

    return result;
}
