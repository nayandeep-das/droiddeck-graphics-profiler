
#ifndef REPORT_GENERATOR_HPP
#define REPORT_GENERATOR_HPP

#include "baseline_comparison.hpp"
#include "correlation_analyzer.hpp"
#include "hypothesis_engine.hpp"
#include "recommendation_engine.hpp"
#include "stutter_analyzer.hpp"

#include <cstddef>
#include <string>
#include <vector>

// Summary of one FPS performance group.
struct FpsGroupSummary {
    std::string name;

    std::size_t sampleCount = 0;

    double averageFps = 0.0;

    double averageRenderSceneMs = 0.0;
    double averageAcquireMs = 0.0;
    double averagePresentMs = 0.0;
    double averageFenceWaitMs = 0.0;
    double averageReleaseMs = 0.0;

    double maximumReleaseLatencyMs = 0.0;

    double averageHeldFrameRatio = 0.0;
    std::size_t heldRatioSampleCount = 0;

    long long totalGpuReleaseWaits = 0;
};

// Overall event and anomaly statistics.
struct PerformanceSummary {
    std::size_t waylandSamplesLoaded = 0;
    std::size_t gameplaySamplesAnalyzed = 0;

    std::size_t stutterEventCount = 0;

    std::size_t fpsDegradationCount = 0;
    std::size_t timingAnomalyCount = 0;
    std::size_t combinedAnomalyCount = 0;

    std::size_t renderSceneAnomalyCount = 0;
    std::size_t presentAnomalyCount = 0;
    std::size_t fenceWaitAnomalyCount = 0;
    std::size_t releaseAnomalyCount = 0;
};

// Complete input required to generate the diagnostic report.
struct DiagnosticReportData {
    PerformanceSummary summary;

    std::vector<FpsGroupSummary> fpsGroups;

    BaselineComparison baseline;

    CorrelationAnalysis correlations;

    HypothesisAnalysis hypotheses;

    RecommendationAnalysis recommendations;

    std::vector<StutterEvent> stutterEvents;
};

// Result of a report-generation operation.
struct ReportGenerationResult {
    bool success = false;

    std::string outputPath;
    std::string errorMessage;
};

// Generates a Markdown diagnostic report from actual analysis results.
ReportGenerationResult generateMarkdownReport(
    const DiagnosticReportData& reportData,
    const std::string& outputPath
);

#endif
