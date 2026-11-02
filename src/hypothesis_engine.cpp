
#include "hypothesis_engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace {

constexpr double TIMING_THRESHOLD_MS = 50.0;
constexpr double LIFECYCLE_WINDOW_SECONDS = 30.0;

struct GroupMetrics {
    std::size_t count = 0;

    double totalFps = 0.0;
    double totalGpuFrames = 0.0;

    double totalRenderAvg = 0.0;
    double totalAcquireAvg = 0.0;
    double totalFenceAvg = 0.0;
    double totalReleaseAvg = 0.0;

    std::size_t renderMaxAnomalies = 0;
    std::size_t acquireMaxAnomalies = 0;
    std::size_t fenceMaxAnomalies = 0;
    std::size_t releaseMaxAnomalies = 0;

    void add(const WaylandLogSample& sample) {
        ++count;

        totalFps += sample.displayedFps;
        totalGpuFrames +=
            static_cast<double>(sample.gpuFramesFromGames);

        totalRenderAvg += sample.renderSceneAvgMs;
        totalAcquireAvg += sample.acquireAvgMs;
        totalFenceAvg += sample.fenceWaitAvgMs;
        totalReleaseAvg += sample.releaseAvgMs;

        if (sample.renderSceneMaxMs > TIMING_THRESHOLD_MS) {
            ++renderMaxAnomalies;
        }

        if (sample.acquireMaxMs > TIMING_THRESHOLD_MS) {
            ++acquireMaxAnomalies;
        }

        if (sample.fenceWaitMaxMs > TIMING_THRESHOLD_MS) {
            ++fenceMaxAnomalies;
        }

        if (sample.releaseMaxMs > TIMING_THRESHOLD_MS) {
            ++releaseMaxAnomalies;
        }
    }

    double average(double total) const {
        return count > 0
            ? total / static_cast<double>(count)
            : 0.0;
    }

    double anomalyRate(std::size_t anomalies) const {
        return count > 0
            ? static_cast<double>(anomalies) /
                  static_cast<double>(count)
            : 0.0;
    }
};

struct EvidenceScore {
    double supportingWeight = 0.0;
    double contradictoryWeight = 0.0;

    std::size_t evidenceCount = 0;

    void support(double weight = 1.0) {
        supportingWeight += weight;
        ++evidenceCount;
    }

    void contradict(double weight = 1.0) {
        contradictoryWeight += weight;
        ++evidenceCount;
    }

    double calculate() const {
    const double total =
        supportingWeight + contradictoryWeight;

    if (total == 0.0 || evidenceCount == 0) {
        return 0.0;
    }

    // Prevent a single evidence item from producing 100/100.
    constexpr double EVIDENCE_ITEMS_FOR_FULL_COVERAGE = 3.0;

    const double quantityFactor = std::min(
        1.0,
        static_cast<double>(evidenceCount) /
            EVIDENCE_ITEMS_FOR_FULL_COVERAGE
    );

    const double evidenceBalance =
        supportingWeight / total;

    return evidenceBalance * quantityFactor * 100.0;

    }
};

// Convert HH:MM:SS.mmm into seconds since midnight.
double timestampToSeconds(const std::string& timestamp) {
    if (timestamp.size() < 8) {
        return -1.0;
    }

    try {
        const int hours = std::stoi(timestamp.substr(0, 2));
        const int minutes = std::stoi(timestamp.substr(3, 2));
        const double seconds = std::stod(timestamp.substr(6));

        return hours * 3600.0 + minutes * 60.0 + seconds;
    } catch (const std::exception&) {
        return -1.0;
    }
}

bool isNearLifecycleTransition(
    const WaylandLogSample& sample,
    const std::vector<double>& transitions
) {
    const double sampleTime =
        timestampToSeconds(sample.timestamp);

    if (sampleTime < 0.0) {
        return false;
    }

    for (const double transitionTime : transitions) {
        if (transitionTime < 0.0) {
            continue;
        }

        double difference =
            std::abs(sampleTime - transitionTime);

        // Handle midnight rollover.
        difference = std::min(difference, 86400.0 - difference);

        if (difference <= LIFECYCLE_WINDOW_SECONDS) {
            return true;
        }
    }

    return false;
}

void addEvidence(
    DiagnosticHypothesis& hypothesis,
    EvidenceScore& score,
    const std::string& evidence,
    bool supports,
    double weight = 1.0
) {
    if (supports) {
        hypothesis.supportingEvidence.push_back(evidence);
        score.support(weight);
    } else {
        hypothesis.contradictoryEvidence.push_back(evidence);
        score.contradict(weight);
    }
}

void finalizeHypothesis(
    DiagnosticHypothesis& hypothesis,
    const EvidenceScore& score
) {
    hypothesis.evidenceScore = score.calculate();

    if (score.evidenceCount >= 3 &&
        hypothesis.evidenceScore >= 75.0) {
        hypothesis.confidence = ConfidenceLevel::High;
    } else if (score.evidenceCount >= 2 &&
               hypothesis.evidenceScore >= 40.0) {
        hypothesis.confidence = ConfidenceLevel::Medium;
    } else {
        hypothesis.confidence = ConfidenceLevel::Low;
    }
}

} // namespace

std::string toString(HypothesisType type) {
    switch (type) {
        case HypothesisType::FrameProduction:
            return "Frame Production Degradation";

        case HypothesisType::ReleaseTiming:
            return "Release Timing Bottleneck";

        case HypothesisType::PresentationLifecycle:
            return "Presentation Lifecycle Interference";

        case HypothesisType::RenderBottleneck:
            return "Render Bottleneck";

        case HypothesisType::Synchronization:
            return "Synchronization Bottleneck";
    }

    return "Unknown";
}

std::string toString(ConfidenceLevel confidence) {
    switch (confidence) {
        case ConfidenceLevel::Low:
            return "LOW";

        case ConfidenceLevel::Medium:
            return "MEDIUM";

        case ConfidenceLevel::High:
            return "HIGH";
    }

    return "UNKNOWN";
}

HypothesisAnalysis analyzeHypotheses(
    const std::vector<WaylandLogSample>& samples
) {
    HypothesisAnalysis analysis;

    GroupMetrics critical;
    GroupMetrics higher;

    std::vector<const WaylandLogSample*> criticalSamples;
    std::vector<double> lifecycleTransitions;

    // Identify lifecycle transitions independently of gameplay filtering.
    for (std::size_t i = 1; i < samples.size(); ++i) {
        if (samples[i].surfaceActive != samples[i - 1].surfaceActive) {
            lifecycleTransitions.push_back(
                timestampToSeconds(samples[i].timestamp)
            );
        }
    }

    // Collect eligible gameplay measurements.
    for (const auto& sample : samples) {
        if (!sample.hasStatsData ||
            !sample.hasPerfData ||
            sample.windowsOpen == 0 ||
            !sample.surfaceActive ||
            sample.gpuFramesFromGames == 0) {
            continue;
        }

        ++analysis.analyzedSamples;

        if (sample.displayedFps < 10.0) {
            critical.add(sample);
            criticalSamples.push_back(&sample);
            ++analysis.criticalSamples;
        } else if (sample.displayedFps >= 30.0) {
            higher.add(sample);
        }
    }

    // 1. Frame production hypothesis.
    {
        DiagnosticHypothesis hypothesis;
        EvidenceScore score;

        hypothesis.type = HypothesisType::FrameProduction;
        hypothesis.title = toString(hypothesis.type);

        if (critical.count > 0 && higher.count > 0) {
            const double criticalGpu =
                critical.average(critical.totalGpuFrames);

            const double higherGpu =
                higher.average(higher.totalGpuFrames);

            const bool lowerProduction =
                criticalGpu < higherGpu * 0.5;

            addEvidence(
                hypothesis,
                score,
                "Critical intervals average " +
                    std::to_string(criticalGpu) +
                    " GPU frames, compared with " +
                    std::to_string(higherGpu) +
                    " in higher-FPS intervals.",
                lowerProduction,
                2.0
            );

            hypothesis.explanation =
                lowerProduction
                ? "Game GPU frame production is substantially lower "
                  "during critical FPS intervals. This is consistent "
                  "with reduced frame production, but does not establish "
                  "why it occurs."
                : "GPU frame production is not substantially lower in "
                  "the critical group. The current data provides limited "
                  "support for this hypothesis.";
        } else {
            hypothesis.explanation =
                "Insufficient critical or higher-FPS samples for comparison.";
        }

        finalizeHypothesis(hypothesis, score);
        analysis.hypotheses.push_back(hypothesis);
    }

    // 2. Release timing hypothesis.
    {
        DiagnosticHypothesis hypothesis;
        EvidenceScore score;

        hypothesis.type = HypothesisType::ReleaseTiming;
        hypothesis.title = toString(hypothesis.type);

        if (critical.count > 0 && higher.count > 0) {
            const double criticalRelease =
                critical.average(critical.totalReleaseAvg);

            const double higherRelease =
                higher.average(higher.totalReleaseAvg);

            const bool higherReleaseLatency =
                criticalRelease > higherRelease * 1.5;

            addEvidence(
                hypothesis,
                score,
                "Critical release average: " +
                    std::to_string(criticalRelease) +
                    " ms; higher-FPS release average: " +
                    std::to_string(higherRelease) + " ms.",
                higherReleaseLatency
            );

            const double criticalAnomalyRate =
                critical.anomalyRate(critical.releaseMaxAnomalies);

            const double higherAnomalyRate =
                higher.anomalyRate(higher.releaseMaxAnomalies);

            addEvidence(
                hypothesis,
                score,
                "Release maximum anomaly rates: critical " +
                    std::to_string(criticalAnomalyRate * 100.0) +
                    "%; higher-FPS " +
                    std::to_string(higherAnomalyRate * 100.0) + "%.",
                criticalAnomalyRate > higherAnomalyRate
            );

            hypothesis.explanation =
                "Release timings are compared between performance groups. "
                "A higher release latency during critical intervals would "
                "support this hypothesis; lower latency would contradict it.";
        } else {
            hypothesis.explanation =
                "Insufficient samples for release timing comparison.";
        }

        finalizeHypothesis(hypothesis, score);
        analysis.hypotheses.push_back(hypothesis);
    }

    // 3. Presentation lifecycle hypothesis.
    {
        DiagnosticHypothesis hypothesis;
        EvidenceScore score;

        hypothesis.type = HypothesisType::PresentationLifecycle;
        hypothesis.title = toString(hypothesis.type);

        std::size_t criticalNearTransition = 0;

        for (const auto* sample : criticalSamples) {
            if (isNearLifecycleTransition(
                    *sample,
                    lifecycleTransitions)) {
                ++criticalNearTransition;
            }
        }

        if (criticalSamples.empty()) {
            hypothesis.explanation =
                "No critical samples are available for lifecycle analysis.";
        } else if (lifecycleTransitions.empty()) {
            addEvidence(
                hypothesis,
                score,
                "No surface-active state transitions were observed.",
                false
            );

            hypothesis.explanation =
                "No observed surface lifecycle transition is available "
                "to support a lifecycle-related explanation.";
        } else {
            const bool transitionAssociation =
                criticalNearTransition > 0;

            addEvidence(
                hypothesis,
                score,
                std::to_string(criticalNearTransition) +
                    " of " + std::to_string(criticalSamples.size()) +
                    " critical samples occurred within 30 seconds "
                    "of a surface lifecycle transition.",
                transitionAssociation,
                2.0
            );

            if (criticalNearTransition < criticalSamples.size()) {
                addEvidence(
                    hypothesis,
                    score,
                    "Some critical samples occurred outside the "
                    "30-second lifecycle window.",
                    false
                );
            }

            hypothesis.explanation =
                transitionAssociation
                ? "Some critical FPS intervals are temporally close to "
                  "surface lifecycle transitions. This is an association, "
                  "not proof that lifecycle changes caused the FPS drop."
                : "Critical FPS intervals were not temporally close to "
                  "the observed surface lifecycle transitions.";
        }

        finalizeHypothesis(hypothesis, score);
        analysis.hypotheses.push_back(hypothesis);
    }

    // 4. Render bottleneck hypothesis.
    {
        DiagnosticHypothesis hypothesis;
        EvidenceScore score;

        hypothesis.type = HypothesisType::RenderBottleneck;
        hypothesis.title = toString(hypothesis.type);

        if (critical.count > 0 && higher.count > 0) {
            const double criticalRender =
                critical.average(critical.totalRenderAvg);

            const double higherRender =
                higher.average(higher.totalRenderAvg);

            const bool higherRenderLatency =
                criticalRender > higherRender * 1.5;

            addEvidence(
                hypothesis,
                score,
                "Render Scene average: critical " +
                    std::to_string(criticalRender) +
                    " ms; higher-FPS " +
                    std::to_string(higherRender) + " ms.",
                higherRenderLatency
            );

            const double criticalAnomalyRate =
                critical.anomalyRate(critical.renderMaxAnomalies);

            const double higherAnomalyRate =
                higher.anomalyRate(higher.renderMaxAnomalies);

            addEvidence(
                hypothesis,
                score,
                "Render Scene maximum anomaly rates: critical " +
                    std::to_string(criticalAnomalyRate * 100.0) +
                    "%; higher-FPS " +
                    std::to_string(higherAnomalyRate * 100.0) + "%.",
                criticalAnomalyRate > higherAnomalyRate
            );

            hypothesis.explanation =
                "Render Scene averages and maximum-timing anomalies "
                "are compared between critical and higher-FPS intervals.";
        } else {
            hypothesis.explanation =
                "Insufficient samples for render timing comparison.";
        }

        finalizeHypothesis(hypothesis, score);
        analysis.hypotheses.push_back(hypothesis);
    }

    // 5. Synchronization hypothesis.
    {
        DiagnosticHypothesis hypothesis;
        EvidenceScore score;

        hypothesis.type = HypothesisType::Synchronization;
        hypothesis.title = toString(hypothesis.type);

        if (critical.count > 0 && higher.count > 0) {
            const double criticalAcquire =
                critical.average(critical.totalAcquireAvg);

            const double higherAcquire =
                higher.average(higher.totalAcquireAvg);

            const double criticalFence =
                critical.average(critical.totalFenceAvg);

            const double higherFence =
                higher.average(higher.totalFenceAvg);

            const bool higherAcquireLatency =
                criticalAcquire > higherAcquire * 1.5;

            const bool higherFenceLatency =
                criticalFence > higherFence * 1.5;

            addEvidence(
                hypothesis,
                score,
                "Acquire average: critical " +
                    std::to_string(criticalAcquire) +
                    " ms; higher-FPS " +
                    std::to_string(higherAcquire) + " ms.",
                higherAcquireLatency
            );

            addEvidence(
                hypothesis,
                score,
                "Fence Wait average: critical " +
                    std::to_string(criticalFence) +
                    " ms; higher-FPS " +
                    std::to_string(higherFence) + " ms.",
                higherFenceLatency
            );

            const std::size_t criticalFenceAnomalies =
                critical.fenceMaxAnomalies +
                critical.acquireMaxAnomalies;

            const std::size_t higherFenceAnomalies =
                higher.fenceMaxAnomalies +
                higher.acquireMaxAnomalies;

            addEvidence(
                hypothesis,
                score,
                "Acquire/Fence maximum anomalies: critical " +
                    std::to_string(criticalFenceAnomalies) +
                    "; higher-FPS " +
                    std::to_string(higherFenceAnomalies) + ".",
                criticalFenceAnomalies > higherFenceAnomalies
            );

            hypothesis.explanation =
                "Acquire and Fence Wait timing measurements are compared "
                "to investigate possible synchronization delays.";
        } else {
            hypothesis.explanation =
                "Insufficient samples for synchronization comparison.";
        }

        finalizeHypothesis(hypothesis, score);
        analysis.hypotheses.push_back(hypothesis);
    }

    // Rank hypotheses by evidence score, highest first.
    std::stable_sort(
        analysis.hypotheses.begin(),
        analysis.hypotheses.end(),
        [](const DiagnosticHypothesis& a,
           const DiagnosticHypothesis& b) {
            return a.evidenceScore > b.evidenceScore;
        }
    );

    return analysis;
}
