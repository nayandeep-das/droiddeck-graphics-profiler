
#include "report_generator.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

void writeSectionHeader(
    std::ofstream& file,
    const std::string& title
) {
    file << "\n## " << title << "\n\n";
}

void writeMetric(
    std::ofstream& file,
    const std::string& name,
    double value,
    const std::string& unit = ""
) {
    file << "- **" << name << ":** "
         << value << unit << "\n";
}

void writeCorrelation(
    std::ofstream& file,
    const std::string& name,
    const CorrelationResult& result
) {
    file << "### " << name << "\n\n";

    file << "- Valid samples: " << result.sampleCount << "\n";

    file << "- Pearson correlation: ";

    if (result.coefficient.has_value()) {
        file << *result.coefficient;
    } else {
        file << "N/A (insufficient or constant data)";
    }

    file << "\n\n";
}

void writeMetricDifference(
    std::ofstream& file,
    const std::string& name,
    const MetricDifference& metric,
    const std::string& unit = ""
) {
    file << "### " << name << "\n\n";

    file << "| Measurement | Value |\n";
    file << "|---|---:|\n";

    file << "| Critical | "
         << metric.criticalValue << unit << " |\n";

    file << "| Higher FPS | "
         << metric.higherValue << unit << " |\n";

    file << "| Difference | "
         << metric.absoluteDifference << unit << " |\n";

    file << "| Percentage difference | ";

    if (metric.percentageDifference.has_value()) {
        file << *metric.percentageDifference << "%";
    } else {
        file << "N/A";
    }

    file << " |\n\n";
}

void writeBaselineMetrics(
    std::ofstream& file,
    const std::string& groupName,
    const BaselineMetrics& metrics
) {
    file << "### " << groupName << "\n\n";

    file << "- Sample count: " << metrics.sampleCount << "\n";

    writeMetric(file, "Average FPS", metrics.averageFps);

    writeMetric(file, "Minimum FPS", metrics.minimumFps);

    writeMetric(file, "Maximum FPS", metrics.maximumFps);

    writeMetric(
        file,
        "Render Scene average",
        metrics.averageRenderSceneMs,
        " ms"
    );

    writeMetric(
        file,
        "Acquire average",
        metrics.averageAcquireMs,
        " ms"
    );

    writeMetric(
        file,
        "Present average",
        metrics.averagePresentMs,
        " ms"
    );

    writeMetric(
        file,
        "Fence Wait average",
        metrics.averageFenceWaitMs,
        " ms"
    );

    writeMetric(
        file,
        "Release average",
        metrics.averageReleaseMs,
        " ms"
    );

    writeMetric(
        file,
        "Maximum release latency",
        metrics.maximumReleaseMs,
        " ms"
    );

    file << "- **Average held-frame ratio:** ";

    if (metrics.heldRatioSampleCount > 0) {
        file << metrics.averageHeldFrameRatio * 100.0 << "%";
    } else {
        file << "N/A";
    }

    file << "\n\n";
}

void writeFpsGroups(
    std::ofstream& file,
    const std::vector<FpsGroupSummary>& groups
) {
    writeSectionHeader(file, "FPS Group Comparison");

    if (groups.empty()) {
        file << "No FPS group data available.\n";
        return;
    }

    file << "| FPS Group | Samples | Avg FPS | Render Scene | Acquire | "
            "Present | Fence Wait | Release |\n";

    file << "|---|---:|---:|---:|---:|---:|---:|---:|\n";

    for (const auto& group : groups) {
        file << "| " << group.name
             << " | " << group.sampleCount
             << " | " << group.averageFps
             << " | " << group.averageRenderSceneMs << " ms"
             << " | " << group.averageAcquireMs << " ms"
             << " | " << group.averagePresentMs << " ms"
             << " | " << group.averageFenceWaitMs << " ms"
             << " | " << group.averageReleaseMs << " ms"
             << " |\n";
    }

    file << "\n";

    for (const auto& group : groups) {
        file << "### " << group.name << "\n\n";

        file << "- Maximum release latency: "
             << group.maximumReleaseLatencyMs << " ms\n";

        file << "- Average held-frame ratio: ";

        if (group.heldRatioSampleCount > 0) {
            file << group.averageHeldFrameRatio * 100.0 << "%";
        } else {
            file << "N/A";
        }

        file << "\n";

        file << "- Held-ratio valid samples: "
             << group.heldRatioSampleCount << "\n";

        file << "- GPU release waits: "
             << group.totalGpuReleaseWaits << "\n\n";
    }
}

void writeBaselineComparison(
    std::ofstream& file,
    const BaselineComparison& baseline
) {
    writeSectionHeader(file, "Baseline Comparison");

    file << "Comparison between critical FPS intervals and higher-FPS "
            "intervals.\n\n";

    writeBaselineMetrics(
        file,
        "Critical FPS Group",
        baseline.critical
    );

    writeBaselineMetrics(
        file,
        "Higher FPS Group",
        baseline.higher
    );

    file << "### Metric Differences\n\n";

    writeMetricDifference(file, "FPS", baseline.fps);

    writeMetricDifference(
        file,
        "Render Scene",
        baseline.renderScene,
        " ms"
    );

    writeMetricDifference(
        file,
        "Acquire",
        baseline.acquire,
        " ms"
    );

    writeMetricDifference(
        file,
        "Present",
        baseline.present,
        " ms"
    );

    writeMetricDifference(
        file,
        "Fence Wait",
        baseline.fenceWait,
        " ms"
    );

    writeMetricDifference(
        file,
        "Release",
        baseline.release,
        " ms"
    );
}

void writeCorrelationAnalysis(
    std::ofstream& file,
    const CorrelationAnalysis& correlation
) {
    writeSectionHeader(file, "Correlation Analysis");

    file << "- Gameplay samples: "
         << correlation.gameplaySampleCount << "\n";

    file << "- Critical samples: "
         << correlation.criticalSampleCount << "\n";

    file << "- Higher-FPS samples: "
         << correlation.higherFpsSampleCount << "\n\n";

    writeCorrelation(
        file,
        "FPS vs GPU Frames from Games",
        correlation.fpsVsGpuFrames
    );

    writeCorrelation(
        file,
        "FPS vs Frames on Screen",
        correlation.fpsVsFramesOnScreen
    );

    writeCorrelation(
        file,
        "FPS vs Held-Frame Ratio",
        correlation.fpsVsHeldFrameRatio
    );

    writeCorrelation(
        file,
        "FPS vs GPU Release Waits",
        correlation.fpsVsGpuReleaseWaits
    );

    file << "**Interpretation note:** FPS and frames on screen are "
            "measured over the same ten-second window. Their strong "
            "correlation is not independent evidence of a bottleneck. "
            "Correlation does not establish causation.\n\n";

    file << "### Critical vs Higher-FPS Group Averages\n\n";

    file << "| Metric | Critical | Higher FPS |\n";
    file << "|---|---:|---:|\n";

    file << "| Average FPS | "
         << correlation.criticalAverageFps << " | "
         << correlation.higherAverageFps << " |\n";

    file << "| GPU frames from games | "
         << correlation.criticalAverageGpuFrames << " | "
         << correlation.higherAverageGpuFrames << " |\n";

    file << "| Frames on screen | "
         << correlation.criticalAverageFramesOnScreen << " | "
         << correlation.higherAverageFramesOnScreen << " |\n";

    file << "| Held-frame ratio | "
         << correlation.criticalAverageHeldRatio * 100.0 << "% | "
         << correlation.higherAverageHeldRatio * 100.0 << "% |\n";

    file << "| GPU release waits | "
         << correlation.criticalAverageGpuReleaseWaits << " | "
         << correlation.higherAverageGpuReleaseWaits << " |\n\n";

    file << "Held-frame ratio valid samples: Critical = "
         << correlation.criticalHeldRatioSamples
         << ", Higher FPS = "
         << correlation.higherHeldRatioSamples << ".\n\n";
}

void writeHypotheses(
    std::ofstream& file,
    const HypothesisAnalysis& analysis
) {
    writeSectionHeader(file, "Diagnostic Hypotheses");

    file << "- Gameplay samples analyzed: "
         << analysis.analyzedSamples << "\n";

    file << "- Critical samples: "
         << analysis.criticalSamples << "\n\n";

    if (analysis.hypotheses.empty()) {
        file << "No diagnostic hypotheses were generated.\n";
        return;
    }

    for (std::size_t i = 0;
         i < analysis.hypotheses.size(); ++i) {

        const auto& hypothesis = analysis.hypotheses[i];

        file << "### " << i + 1 << ". "
             << hypothesis.title << "\n\n";

        file << "- Category: "
             << toString(hypothesis.type) << "\n";

        file << "- Confidence: "
             << toString(hypothesis.confidence) << "\n";

        file << "- Evidence score: "
             << hypothesis.evidenceScore << "/100\n\n";

        file << "**Explanation:** "
             << hypothesis.explanation << "\n\n";

        file << "**Supporting evidence:**\n\n";

        if (hypothesis.supportingEvidence.empty()) {
            file << "- None identified\n";
        } else {
            for (const auto& evidence :
                 hypothesis.supportingEvidence) {
                file << "- " << evidence << "\n";
            }
        }

        file << "\n**Contradictory evidence:**\n\n";

        if (hypothesis.contradictoryEvidence.empty()) {
            file << "- None identified\n";
        } else {
            for (const auto& evidence :
                 hypothesis.contradictoryEvidence) {
                file << "- " << evidence << "\n";
            }
        }

        file << "\n";
    }

    file << "**Important:** Evidence scores are heuristic rankings, "
            "not probabilities. Temporal association and correlation "
            "do not establish causation.\n\n";
}

void writeRecommendations(
    std::ofstream& file,
    const RecommendationAnalysis& analysis
) {
    writeSectionHeader(file, "Diagnostic Recommendations");

    file << "- Total recommendations: "
         << analysis.totalRecommendations << "\n";

    file << "- High priority: "
         << analysis.highPriorityCount << "\n";

    file << "- Medium priority: "
         << analysis.mediumPriorityCount << "\n";

    file << "- Low priority: "
         << analysis.lowPriorityCount << "\n\n";

    if (analysis.recommendations.empty()) {
        file << "No recommendations were generated.\n";
        return;
    }

    for (std::size_t i = 0;
         i < analysis.recommendations.size(); ++i) {

        const auto& recommendation = analysis.recommendations[i];

        file << "### " << i + 1 << ". ["
             << toString(recommendation.priority) << "] "
             << recommendation.title << "\n\n";

        file << "- Related hypothesis: "
             << toString(recommendation.relatedHypothesis) << "\n\n";

        file << "**Rationale:** "
             << recommendation.rationale << "\n\n";

        file << "**Investigation steps:**\n\n";

        for (const auto& step : recommendation.investigationSteps) {
            file << "- " << step << "\n";
        }

        file << "\n**Validation experiments:**\n\n";

        for (const auto& experiment :
             recommendation.validationExperiments) {
            file << "- " << experiment << "\n";
        }

        file << "\n";
    }

    file << "Recommendations are investigation priorities, "
            "not confirmed root causes.\n\n";
}

void writeStutterEvents(
    std::ofstream& file,
    const std::vector<StutterEvent>& events
) {
    writeSectionHeader(file, "Detected Stutter Events");

    if (events.empty()) {
        file << "No stutter events were detected.\n";
        return;
    }

    file << "| Timestamp | Severity | FPS | FPS Degradation | "
            "Timing Anomaly |\n";

    file << "|---|---|---:|---|---|\n";

    for (const auto& event : events) {
        file << "| " << event.timestamp
             << " | " << toString(event.severity)
             << " | " << event.displayedFps
             << " | " << (event.fpsDegradation ? "Yes" : "No")
             << " | " << (event.timingAnomaly ? "Yes" : "No")
             << " |\n";
    }

    file << "\n";

    for (const auto& event : events) {
        file << "### " << event.timestamp
             << " — " << toString(event.severity) << "\n\n";

        file << "- Displayed FPS: "
             << event.displayedFps << "\n";

        file << "- Render Scene maximum: "
             << event.renderSceneMaxMs << " ms\n";

        file << "- Present maximum: "
             << event.presentMaxMs << " ms\n";

        file << "- Fence Wait maximum: "
             << event.fenceWaitMaxMs << " ms\n";

        file << "- Release maximum: "
             << event.releaseMaxMs << " ms\n";

        file << "- Reasons:\n";

        if (event.reasons.empty()) {
            file << "  - No reason details available.\n";
        } else {
            for (const auto& reason : event.reasons) {
                file << "  - " << reason << "\n";
            }
        }

        file << "\n";
    }
}

void writeLimitations(std::ofstream& file) {
    writeSectionHeader(file, "Limitations and Interpretation");

    file << "- The analysis is based on the supplied telemetry session.\n";

    file << "- Ten-second aggregated metrics can hide short-lived events.\n";

    file << "- Correlation does not establish causation.\n";

    file << "- FPS and frame-count metrics may be coupled by their "
            "measurement windows.\n";

    file << "- Hypothesis evidence scores are heuristic rankings, "
            "not statistical probabilities.\n";

    file << "- More gameplay sessions and independent per-frame "
            "measurements are needed to establish a root cause.\n\n";
}

} // namespace

ReportGenerationResult generateMarkdownReport(
    const DiagnosticReportData& reportData,
    const std::string& outputPath
) {
    ReportGenerationResult result;
    result.outputPath = outputPath;

    try {
        if (outputPath.empty()) {
            throw std::invalid_argument(
                "Report output path cannot be empty."
            );
        }

        const std::filesystem::path path(outputPath);

        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(
                path.parent_path()
            );
        }

        std::ofstream file(
            path,
            std::ios::out | std::ios::trunc
        );

        if (!file.is_open()) {
            throw std::runtime_error(
                "Unable to open report output file: " + outputPath
            );
        }

        file << std::fixed << std::setprecision(2);

        file << "# DroidDeck Graphics Profiler\n\n";

        file << "## Wayland Diagnostic Performance Report\n\n";

        file << "**Report type:** Automated diagnostic report  \n";
        file << "**Analysis mode:** Wayland / Gamescope telemetry  \n";
        file << "**Generated by:** DroidDeck Graphics Profiler\n";

        // Executive summary
        writeSectionHeader(file, "Executive Summary");

        file << "This report summarizes the results of the "
                "Wayland performance analysis.\n\n";

        file << "- Wayland samples loaded: "
             << reportData.summary.waylandSamplesLoaded << "\n";

        file << "- Gameplay samples analyzed: "
             << reportData.summary.gameplaySamplesAnalyzed << "\n";

        file << "- Stutter events detected: "
             << reportData.summary.stutterEventCount << "\n";

        file << "- FPS degradation events: "
             << reportData.summary.fpsDegradationCount << "\n";

        file << "- Timing anomaly events: "
             << reportData.summary.timingAnomalyCount << "\n";

        file << "- Combined anomaly events: "
             << reportData.summary.combinedAnomalyCount << "\n\n";

        // Performance overview
        writeSectionHeader(file, "Performance Overview");

        file << "| Subsystem | Anomaly Events |\n";
        file << "|---|---:|\n";

        file << "| Render Scene | "
             << reportData.summary.renderSceneAnomalyCount << " |\n";

        file << "| Present | "
             << reportData.summary.presentAnomalyCount << " |\n";

        file << "| Fence Wait | "
             << reportData.summary.fenceWaitAnomalyCount << " |\n";

        file << "| Release | "
             << reportData.summary.releaseAnomalyCount << " |\n\n";

        // Detailed analysis
        writeFpsGroups(file, reportData.fpsGroups);

        writeBaselineComparison(file, reportData.baseline);

        writeCorrelationAnalysis(file, reportData.correlations);

        writeHypotheses(file, reportData.hypotheses);

        writeRecommendations(file, reportData.recommendations);

        writeStutterEvents(file, reportData.stutterEvents);

        writeLimitations(file);

        writeSectionHeader(file, "Conclusion");

        file << "The report presents measured performance data, "
                "diagnostic hypotheses, and recommended investigation "
                "steps. The findings should be validated with additional "
                "measurements before assigning a definitive root cause.\n";

        file.flush();

        if (!file.good()) {
            throw std::runtime_error(
                "An error occurred while writing the diagnostic report."
            );
        }

        file.close();

        result.success = true;
    }
    catch (const std::exception& error) {
        result.success = false;
        result.errorMessage = error.what();
    }

    return result;
}
