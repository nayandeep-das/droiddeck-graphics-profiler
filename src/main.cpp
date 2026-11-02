
#include "csv_reader.hpp"
#include "statistics.hpp"
#include "baseline_comparison.hpp"
#include "correlation_analyzer.hpp"
#include "stutter_analyzer.hpp"
#include "hypothesis_engine.hpp"
#include "recommendation_engine.hpp"
#include "report_generator.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {

    // Wayland analysis mode
    if (argc == 2 && std::string(argv[1]) == "--wayland") {
        try {
            const auto samples = loadWaylandLog("data/wayland.log");
            const auto events = detectStutters(samples);
            const auto baseline = compareBaselines(samples);
            const auto correlation = analyzeCorrelations(samples);
            const auto hypotheses = analyzeHypotheses(samples);

            const auto recommendations =
                generateRecommendations(hypotheses);

            // FPS group statistics
            struct FpsGroup {
                std::string name;
                std::size_t count = 0;

                double totalFps = 0.0;
                double totalReleaseAvgMs = 0.0;
                double maximumReleaseLatencyMs = 0.0;
                double totalHeldRatio = 0.0;

                double totalRenderSceneAvgMs = 0.0;
                double totalAcquireAvgMs = 0.0;
                double totalPresentAvgMs = 0.0;
                double totalFenceWaitAvgMs = 0.0;

                std::size_t heldRatioSamples = 0;
                long long totalGpuReleaseWaits = 0;
            };

            std::array<FpsGroup, 4> groups{{
                {"Critical (<10 FPS)"},
                {"Low (10-<15 FPS)"},
                {"Intermediate (15-<30 FPS)"},
                {"Higher (>=30 FPS)"}
            }};

            std::size_t analyzedSampleCount = 0;

            for (const auto& sample : samples) {
                if (!sample.hasStatsData ||
                    !sample.hasPerfData ||
                    sample.windowsOpen == 0 ||
                    !sample.surfaceActive ||
                    sample.gpuFramesFromGames == 0) {
                    continue;
                }

                ++analyzedSampleCount;

                std::size_t groupIndex = 0;

                if (sample.displayedFps < 10.0) {
                    groupIndex = 0;
                } else if (sample.displayedFps < 15.0) {
                    groupIndex = 1;
                } else if (sample.displayedFps < 30.0) {
                    groupIndex = 2;
                } else {
                    groupIndex = 3;
                }

                auto& group = groups[groupIndex];

                ++group.count;

                group.totalFps += sample.displayedFps;

                group.totalRenderSceneAvgMs += sample.renderSceneAvgMs;
                group.totalAcquireAvgMs += sample.acquireAvgMs;
                group.totalPresentAvgMs += sample.presentAvgMs;
                group.totalFenceWaitAvgMs += sample.fenceWaitAvgMs;

                group.totalReleaseAvgMs += sample.releaseAvgMs;
                group.maximumReleaseLatencyMs =
                    std::max(group.maximumReleaseLatencyMs, sample.releaseMaxMs);
                group.totalGpuReleaseWaits += sample.gpuReleaseWaits;

                if (sample.releaseFrameCount > 0) {
                    group.totalHeldRatio +=
                        static_cast<double>(sample.releaseHeldFrames) /
                        static_cast<double>(sample.releaseFrameCount);

                    ++group.heldRatioSamples;
                }
            }

            // Stutter and subsystem anomaly counts
            std::size_t fpsDegradationCount = 0;
            std::size_t timingAnomalyCount = 0;
            std::size_t combinedCount = 0;

            std::size_t renderSceneCount = 0;
            std::size_t presentCount = 0;
            std::size_t fenceWaitCount = 0;
            std::size_t releaseCount = 0;

            for (const auto& event : events) {
                if (event.fpsDegradation) {
                    ++fpsDegradationCount;
                }

                if (event.timingAnomaly) {
                    ++timingAnomalyCount;
                }

                if (event.fpsDegradation && event.timingAnomaly) {
                    ++combinedCount;
                }

                if (event.renderSceneMaxMs > 50.0) {
                    ++renderSceneCount;
                }

                if (event.presentMaxMs > 50.0) {
                    ++presentCount;
                }

                if (event.fenceWaitMaxMs > 50.0) {
                    ++fenceWaitCount;
                }

                if (event.releaseMaxMs > 50.0) {
                    ++releaseCount;
                }
            }

            // Overall performance summary
            std::cout << "Wayland samples loaded: "
                      << samples.size() << '\n';

            std::cout << "Stutter events detected: "
                      << events.size() << "\n\n";

            std::cout << "========== WAYLAND PERFORMANCE SUMMARY ==========\n\n";

            std::cout << "Gameplay samples analyzed:    "
                      << analyzedSampleCount << '\n';

            std::cout << "FPS degradation events:       "
                      << fpsDegradationCount << '\n';

            std::cout << "Timing anomaly events:        "
                      << timingAnomalyCount << '\n';

            std::cout << "Combined events:              "
                      << combinedCount << '\n';

            std::cout << "\nSubsystem timing anomalies:\n";

            std::cout << "  Render Scene:               "
                      << renderSceneCount << '\n';

            std::cout << "  Present:                    "
                      << presentCount << '\n';

            std::cout << "  Fence Wait:                 "
                      << fenceWaitCount << '\n';

            std::cout << "  Release:                    "
                      << releaseCount << '\n';

            std::cout << "\n==================================================\n\n";

            // FPS group comparison
            std::cout << "========== FPS GROUP COMPARISON ==========\n\n";

            std::cout << std::fixed << std::setprecision(2);

            for (const auto& group : groups) {
                std::cout << group.name << '\n';

                std::cout << "  Samples:                  "
                          << group.count << '\n';

                if (group.count == 0) {
                    std::cout << "  No samples in this group.\n\n";
                    continue;
                }

                const double count =
                    static_cast<double>(group.count);

                std::cout << "  Average FPS:              "
                          << group.totalFps / count << '\n';

                std::cout << "  Render Scene average:     "
                          << group.totalRenderSceneAvgMs / count << " ms\n";

                std::cout << "  Acquire average:           "
                          << group.totalAcquireAvgMs / count << " ms\n";

                std::cout << "  Present average:           "
                          << group.totalPresentAvgMs / count << " ms\n";

                std::cout << "  Fence Wait average:        "
                          << group.totalFenceWaitAvgMs / count << " ms\n";

                std::cout << "  Release average latency:  "
                          << group.totalReleaseAvgMs / count
                          << " ms\n";

                std::cout << "  Release maximum latency:  "
                          << group.maximumReleaseLatencyMs
                          << " ms\n";

                if (group.heldRatioSamples > 0) {
                    std::cout << "  Average held-frame ratio: "
                              << (group.totalHeldRatio /
                                  static_cast<double>(
                                      group.heldRatioSamples)) * 100.0
                              << "%\n";
                } else {
                    std::cout << "  Average held-frame ratio: N/A\n";
                }

                std::cout << "  GPU release waits:        "
                          << group.totalGpuReleaseWaits << '\n';

                std::cout << '\n';
            }

            std::cout << "==========================================\n\n";

            // Critical vs Higher FPS baseline comparison
            std::cout << "========== BASELINE COMPARISON ==========\n\n";

            auto printMetric = [](const std::string& name,
                                  const MetricDifference& metric) {
                std::cout << name << ":\n";
                std::cout << "  Critical: " << metric.criticalValue << '\n';
                std::cout << "  Higher:   " << metric.higherValue << '\n';
                std::cout << "  Difference: " << metric.absoluteDifference << '\n';
                std::cout << "  Percentage: ";
                if (metric.percentageDifference.has_value()) {
                    std::cout << *metric.percentageDifference << "%\n";
                } else {
                    std::cout << "N/A\n";
                }
                std::cout << '\n';
            };

            std::cout << "Critical samples: "
                      << baseline.critical.sampleCount << '\n';
            std::cout << "Higher FPS samples: "
                      << baseline.higher.sampleCount << "\n\n";

            std::cout << "FPS RANGE:\n";
            std::cout << "  Critical: " << baseline.critical.minimumFps
                      << " - " << baseline.critical.maximumFps << '\n';
            std::cout << "  Higher:   " << baseline.higher.minimumFps
                      << " - " << baseline.higher.maximumFps << "\n\n";

            printMetric("FPS average", baseline.fps);
            printMetric("Render Scene (ms)", baseline.renderScene);
            printMetric("Acquire (ms)", baseline.acquire);
            printMetric("Present (ms)", baseline.present);
            printMetric("Fence Wait (ms)", baseline.fenceWait);
            printMetric("Release (ms)", baseline.release);

            std::cout << "=========================================\n\n";

            // FPS correlation analysis
            std::cout << "========== CORRELATION ANALYSIS ==========\n\n";

            // Show enough precision to distinguish near-perfect correlations.
            std::cout << std::fixed << std::setprecision(4);

            auto printCorrelation = [](
                const std::string& name,
                const CorrelationResult& result
            ) {
                std::cout << name << ":\n";
                std::cout << "  Valid samples: " << result.sampleCount << '\n';
                std::cout << "  Pearson r: ";

                if (result.coefficient.has_value()) {
                    std::cout << *result.coefficient << '\n';
                } else {
                    std::cout << "N/A (insufficient or constant data)\n";
                }

                std::cout << '\n';
            };

            std::cout << "Gameplay samples: "
                      << correlation.gameplaySampleCount << '\n';
            std::cout << "Critical samples: "
                      << correlation.criticalSampleCount << '\n';
            std::cout << "Higher FPS samples: "
                      << correlation.higherFpsSampleCount << "\n\n";

            printCorrelation(
                "FPS vs GPU frames from games",
                correlation.fpsVsGpuFrames
            );

            printCorrelation(
                "FPS vs frames on screen",
                correlation.fpsVsFramesOnScreen
            );

            std::cout
                << "Interpretation note: FPS and frames on screen are both "
                   "measured over the same 10-second window. Their strong "
                   "correlation is expected and is not independent evidence "
                   "of a bottleneck.\n\n";

            printCorrelation(
                "FPS vs held-frame ratio",
                correlation.fpsVsHeldFrameRatio
            );

            printCorrelation(
                "FPS vs GPU release waits",
                correlation.fpsVsGpuReleaseWaits
            );

            std::cout << "CRITICAL vs HIGHER FPS GROUPS:\n\n";

            std::cout << "Average FPS:\n";
            std::cout << "  Critical: " << correlation.criticalAverageFps << '\n';
            std::cout << "  Higher:   " << correlation.higherAverageFps << "\n\n";

            std::cout << "Average GPU frames from games:\n";
            std::cout << "  Critical: "
                      << correlation.criticalAverageGpuFrames << '\n';
            std::cout << "  Higher:   "
                      << correlation.higherAverageGpuFrames << "\n\n";

            std::cout << "Average frames on screen:\n";
            std::cout << "  Critical: "
                      << correlation.criticalAverageFramesOnScreen << '\n';
            std::cout << "  Higher:   "
                      << correlation.higherAverageFramesOnScreen << "\n\n";

            std::cout << "Average held-frame ratio:\n";
            std::cout << "  Critical: "
                      << correlation.criticalAverageHeldRatio * 100.0
                      << "% (" << correlation.criticalHeldRatioSamples
                      << " valid samples)\n";
            std::cout << "  Higher:   "
                      << correlation.higherAverageHeldRatio * 100.0
                      << "% (" << correlation.higherHeldRatioSamples
                      << " valid samples)\n\n";

            std::cout << "Average GPU release waits:\n";
            std::cout << "  Critical: "
                      << correlation.criticalAverageGpuReleaseWaits << '\n';
            std::cout << "  Higher:   "
                      << correlation.higherAverageGpuReleaseWaits << "\n\n";

            std::cout << "==========================================\n\n";

            // Diagnostic hypothesis analysis
            std::cout << "========== DIAGNOSTIC HYPOTHESES ==========\n\n";

            std::cout << "Gameplay samples analyzed: "
                      << hypotheses.analyzedSamples << '\n';

            std::cout << "Critical samples: "
                      << hypotheses.criticalSamples << "\n\n";

            for (std::size_t i = 0;
                 i < hypotheses.hypotheses.size(); ++i) {

                const auto& hypothesis = hypotheses.hypotheses[i];

                std::cout << i + 1 << ". "
                          << hypothesis.title << '\n';

                std::cout << "   Category: "
                          << toString(hypothesis.type) << '\n';

                std::cout << "   Confidence: "
                          << toString(hypothesis.confidence) << '\n';

                std::cout << "   Evidence score: "
                          << std::fixed << std::setprecision(2)
                          << hypothesis.evidenceScore << "/100\n";

                std::cout << "   Explanation: "
                          << hypothesis.explanation << '\n';

                std::cout << "   Supporting evidence:\n";

                if (hypothesis.supportingEvidence.empty()) {
                    std::cout << "     - None identified\n";
                } else {
                    for (const auto& evidence :
                         hypothesis.supportingEvidence) {
                        std::cout << "     - " << evidence << '\n';
                    }
                }

                std::cout << "   Contradictory evidence:\n";

                if (hypothesis.contradictoryEvidence.empty()) {
                    std::cout << "     - None identified\n";
                } else {
                    for (const auto& evidence :
                         hypothesis.contradictoryEvidence) {
                        std::cout << "     - " << evidence << '\n';
                    }
                }

                std::cout << '\n';
            }

            std::cout
                << "Note: Evidence scores are heuristic rankings, "
                   "not probabilities. Correlation and temporal "
                   "association do not establish causation.\n";

            std::cout
                << "===========================================\n\n";

            // Actionable diagnostic recommendations
            std::cout
                << "========== DIAGNOSTIC RECOMMENDATIONS ==========\n\n";

            std::cout << "Total recommendations: "
                      << recommendations.totalRecommendations << '\n';

            std::cout << "High priority:   "
                      << recommendations.highPriorityCount << '\n';

            std::cout << "Medium priority: "
                      << recommendations.mediumPriorityCount << '\n';

            std::cout << "Low priority:    "
                      << recommendations.lowPriorityCount << "\n\n";

            for (std::size_t i = 0;
                 i < recommendations.recommendations.size(); ++i) {

                const auto& recommendation =
                    recommendations.recommendations[i];

                std::cout << i + 1 << ". ["
                          << toString(recommendation.priority)
                          << "] " << recommendation.title << '\n';

                std::cout << "   Related hypothesis: "
                          << toString(recommendation.relatedHypothesis)
                          << '\n';

                std::cout << "   Rationale: "
                          << recommendation.rationale << '\n';

                std::cout << "   Investigation steps:\n";

                for (const auto& step :
                     recommendation.investigationSteps) {
                    std::cout << "      - " << step << '\n';
                }

                std::cout << "   Validation experiments:\n";

                for (const auto& experiment :
                     recommendation.validationExperiments) {
                    std::cout << "      - " << experiment << '\n';
                }

                std::cout << '\n';
            }

            std::cout
                << "Note: Recommendations are investigation priorities, "
                   "not confirmed root causes.\n";

            std::cout
                << "================================================\n\n";

            // Restore the report's usual numeric precision.
            std::cout << std::setprecision(2);

            // Individual detected events
            std::cout << "========== DETECTED STUTTER EVENTS ==========\n\n";

            for (const auto& event : events) {
                std::cout << "[" << toString(event.severity) << "] "
                          << event.timestamp
                          << " | FPS: " << event.displayedFps
                          << '\n';

                for (const auto& reason : event.reasons) {
                    std::cout << "  - " << reason << '\n';
                }

                std::cout << '\n';
            }

            // Generate a Markdown report from the results already computed.
            DiagnosticReportData reportData;

            reportData.summary.waylandSamplesLoaded = samples.size();
            reportData.summary.gameplaySamplesAnalyzed = analyzedSampleCount;
            reportData.summary.stutterEventCount = events.size();
            reportData.summary.fpsDegradationCount = fpsDegradationCount;
            reportData.summary.timingAnomalyCount = timingAnomalyCount;
            reportData.summary.combinedAnomalyCount = combinedCount;
            reportData.summary.renderSceneAnomalyCount = renderSceneCount;
            reportData.summary.presentAnomalyCount = presentCount;
            reportData.summary.fenceWaitAnomalyCount = fenceWaitCount;
            reportData.summary.releaseAnomalyCount = releaseCount;

            for (const auto& group : groups) {
                FpsGroupSummary groupSummary;
                groupSummary.name = group.name;
                groupSummary.sampleCount = group.count;

                if (group.count > 0) {
                    const double count = static_cast<double>(group.count);
                    groupSummary.averageFps = group.totalFps / count;
                    groupSummary.averageRenderSceneMs =
                        group.totalRenderSceneAvgMs / count;
                    groupSummary.averageAcquireMs =
                        group.totalAcquireAvgMs / count;
                    groupSummary.averagePresentMs =
                        group.totalPresentAvgMs / count;
                    groupSummary.averageFenceWaitMs =
                        group.totalFenceWaitAvgMs / count;
                    groupSummary.averageReleaseMs =
                        group.totalReleaseAvgMs / count;
                }

                groupSummary.maximumReleaseLatencyMs =
                    group.maximumReleaseLatencyMs;
                groupSummary.heldRatioSampleCount = group.heldRatioSamples;

                if (group.heldRatioSamples > 0) {
                    groupSummary.averageHeldFrameRatio =
                        group.totalHeldRatio /
                        static_cast<double>(group.heldRatioSamples);
                }

                groupSummary.totalGpuReleaseWaits =
                    group.totalGpuReleaseWaits;

                reportData.fpsGroups.push_back(groupSummary);
            }

            reportData.baseline = baseline;
            reportData.correlations = correlation;
            reportData.hypotheses = hypotheses;
            reportData.recommendations = recommendations;
            reportData.stutterEvents = events;

            const auto reportResult = generateMarkdownReport(
                reportData,
                "reports/wayland_diagnostic_report.md"
            );

            if (reportResult.success) {
                std::cout << "Markdown diagnostic report generated: "
                          << reportResult.outputPath << "\n\n";
            } else {
                std::cerr << "Warning: Could not generate Markdown report: "
                          << reportResult.errorMessage << '\n';
            }

        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << '\n';
            return 1;
        }

        return 0;
    }

    // Existing frame-time analysis mode
    double thresholdMs = 50.0;

    if (argc == 3 && std::string(argv[1]) == "--threshold") {
        try {
            std::size_t parsedCharacters = 0;

            thresholdMs = std::stod(argv[2], &parsedCharacters);

            if (parsedCharacters != std::string(argv[2]).size() ||
                thresholdMs <= 0.0) {
                std::cerr
                    << "Error: Threshold must be a positive number.\n";
                return 1;
            }

        } catch (const std::exception&) {
            std::cerr << "Error: Invalid threshold value.\n";
            return 1;
        }

    } else if (argc != 1) {
        std::cerr
            << "Usage:\n"
            << "  frame_analyzer\n"
            << "  frame_analyzer --threshold milliseconds\n"
            << "  frame_analyzer --wayland\n";

        return 1;
    }

    std::vector<double> frameTimes;

    if (!loadFrameTimes("data/frame_times.csv", frameTimes)) {
        return 1;
    }

    try {
        const FrameStatistics stats =
            calculateStatistics(frameTimes, thresholdMs);

        std::cout << "Frames loaded: "
                  << frameTimes.size() << '\n';

        std::cout << "Average frame time: "
                  << stats.averageFrameTime << " ms\n";

        std::cout << "Maximum frame time: "
                  << stats.maximumFrameTime << " ms\n";

        std::cout << "Estimated average FPS: "
                  << stats.averageFps << '\n';

        std::cout << "Stutter threshold: "
                  << thresholdMs << " ms\n";

        std::cout << "Stutters: "
                  << stats.stutterCount << '\n';

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
