
#include "report_generator.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return {};
    }

    std::ostringstream content;
    content << file.rdbuf();

    return content.str();
}

void cleanTestDirectory(
    const std::filesystem::path& directory
) {
    std::error_code error;
    std::filesystem::remove_all(directory, error);
}

} // namespace

int main() {
    const auto testDirectory =
        std::filesystem::temp_directory_path() /
        "droiddeck_report_generator_tests";

    const auto reportPath =
        testDirectory / "nested" / "diagnostic_report.md";

    cleanTestDirectory(testDirectory);

    // Test 1: Empty output path should fail.
    {
        DiagnosticReportData data;

        const auto result =
            generateMarkdownReport(data, "");

        assert(!result.success);
        assert(!result.errorMessage.empty());

        std::cout << "[PASS] Empty output path handling\n";
    }

    // Test 2: Report generation with empty analysis data.
    {
        DiagnosticReportData data;

        const auto result =
            generateMarkdownReport(data, reportPath.string());

        assert(result.success);
        assert(std::filesystem::exists(reportPath));

        const std::string content = readFile(reportPath);

        assert(!content.empty());
        assert(content.find("# DroidDeck Graphics Profiler") !=
               std::string::npos);

        assert(content.find("Executive Summary") !=
               std::string::npos);

        assert(content.find("Baseline Comparison") !=
               std::string::npos);

        assert(content.find("Correlation Analysis") !=
               std::string::npos);

        assert(content.find("Diagnostic Hypotheses") !=
               std::string::npos);

        assert(content.find("Diagnostic Recommendations") !=
               std::string::npos);

        assert(content.find("Detected Stutter Events") !=
               std::string::npos);

        std::cout << "[PASS] Empty analysis report generation\n";
    }

    // Test 3: Directory creation.
    {
        const auto directory =
            testDirectory / "automatic_directory";

        const auto path =
            directory / "report.md";

        DiagnosticReportData data;

        const auto result =
            generateMarkdownReport(data, path.string());

        assert(result.success);
        assert(std::filesystem::exists(directory));
        assert(std::filesystem::exists(path));

        std::cout << "[PASS] Automatic directory creation\n";
    }

    // Test 4: Actual summary values appear in the report.
    {
        DiagnosticReportData data;

        data.summary.waylandSamplesLoaded = 88;
        data.summary.gameplaySamplesAnalyzed = 85;
        data.summary.stutterEventCount = 67;
        data.summary.fpsDegradationCount = 7;
        data.summary.timingAnomalyCount = 61;
        data.summary.combinedAnomalyCount = 1;

        data.summary.renderSceneAnomalyCount = 4;
        data.summary.presentAnomalyCount = 0;
        data.summary.fenceWaitAnomalyCount = 2;
        data.summary.releaseAnomalyCount = 59;

        const auto result =
            generateMarkdownReport(data, reportPath.string());

        assert(result.success);

        const std::string content = readFile(reportPath);

        assert(content.find("Wayland samples loaded: 88") !=
               std::string::npos);

        assert(content.find("Gameplay samples analyzed: 85") !=
               std::string::npos);

        assert(content.find("Stutter events detected: 67") !=
               std::string::npos);

        assert(content.find("FPS degradation events: 7") !=
               std::string::npos);

        assert(content.find("Timing anomaly events: 61") !=
               std::string::npos);

        assert(content.find("Release | 59") !=
               std::string::npos);

        std::cout << "[PASS] Performance summary values\n";
    }

    // Test 5: FPS group summary appears in the report.
    {
        DiagnosticReportData data;

        FpsGroupSummary group;

        group.name = "Critical (<10 FPS)";
        group.sampleCount = 5;
        group.averageFps = 4.04;
        group.averageRenderSceneMs = 5.64;
        group.averageAcquireMs = 0.15;
        group.averagePresentMs = 1.28;
        group.averageFenceWaitMs = 3.43;
        group.averageReleaseMs = 5.17;

        group.maximumReleaseLatencyMs = 43.46;
        group.averageHeldFrameRatio = 0.3984;
        group.heldRatioSampleCount = 5;
        group.totalGpuReleaseWaits = 0;

        data.fpsGroups.push_back(group);

        const auto result =
            generateMarkdownReport(data, reportPath.string());

        assert(result.success);

        const std::string content = readFile(reportPath);

        assert(content.find("Critical (<10 FPS)") !=
               std::string::npos);

        assert(content.find("5.64 ms") !=
               std::string::npos);

        assert(content.find("43.46 ms") !=
               std::string::npos);

        std::cout << "[PASS] FPS group report generation\n";
    }

    // Test 6: Missing correlation values are handled.
    {
        DiagnosticReportData data;

        const auto result =
            generateMarkdownReport(data, reportPath.string());

        assert(result.success);

        const std::string content = readFile(reportPath);

        assert(content.find(
            "N/A (insufficient or constant data)"
        ) != std::string::npos);

        std::cout << "[PASS] Missing correlation handling\n";
    }

    // Test 7: Stutter events appear in the report.
    {
        DiagnosticReportData data;

        StutterEvent event;

        event.timestamp = "18:25:05.669";
        event.displayedFps = 1.10;
        event.severity = StutterSeverity::Critical;
        event.fpsDegradation = true;
        event.timingAnomaly = false;

        event.renderSceneMaxMs = 6.0;
        event.presentMaxMs = 1.5;
        event.fenceWaitMaxMs = 3.0;
        event.releaseMaxMs = 10.0;

        event.reasons.push_back(
            "Critically low displayed FPS"
        );

        data.stutterEvents.push_back(event);

        const auto result =
            generateMarkdownReport(data, reportPath.string());

        assert(result.success);

        const std::string content = readFile(reportPath);

        assert(content.find("18:25:05.669") !=
               std::string::npos);

        assert(content.find("CRITICAL") !=
               std::string::npos);

        assert(content.find("Critically low displayed FPS") !=
               std::string::npos);

        std::cout << "[PASS] Stutter event report generation\n";
    }

    // Test 8: File generation result contains output path.
    {
        DiagnosticReportData data;

        const auto result =
            generateMarkdownReport(data, reportPath.string());

        assert(result.success);
        assert(result.outputPath == reportPath.string());
        assert(result.errorMessage.empty());

        std::cout << "[PASS] Report generation result\n";
    }

    cleanTestDirectory(testDirectory);

    std::cout << "\nAll 8 report generator tests passed.\n";

    return 0;
}
