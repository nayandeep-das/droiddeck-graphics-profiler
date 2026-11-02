
#ifndef WAYLAND_LOG_PARSER_H
#define WAYLAND_LOG_PARSER_H

#include <fstream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>

// Strict floating-point conversion.
// Rejects trailing characters, NaN and infinity.
inline double parseDoubleStrict(const std::string& value) {
    std::size_t parsedCharacters = 0;

    const double result = std::stod(value, &parsedCharacters);

    if (parsedCharacters != value.size() ||
        !std::isfinite(result)) {
        throw std::invalid_argument(
            "Invalid numeric value: " + value
        );
    }

    return result;
}

struct WaylandLogSample {
    std::string timestamp;

    bool hasStatsData = false;
    bool hasPerfData = false;

    double displayedFps = 0.0;
    long long framesOnScreen = 0;
    long long gpuFramesFromGames = 0;
    long long windowsOpen = 0;

    bool surfaceActive = false;

    double renderSceneAvgMs = 0.0;
    double renderSceneMaxMs = 0.0;

    double acquireAvgMs = 0.0;
    double acquireMaxMs = 0.0;

    double presentAvgMs = 0.0;
    double presentMaxMs = 0.0;

    double fenceWaitAvgMs = 0.0;
    double fenceWaitMaxMs = 0.0;

    double releaseAvgMs = 0.0;
    double releaseMaxMs = 0.0;

    long long releaseFrameCount = 0;
    long long releaseHeldFrames = 0;
    long long gpuReleaseWaits = 0;
};

struct SurfaceLifecycleEvent {
    std::string timestamp;
    bool attached = false;
};

inline std::vector<WaylandLogSample> loadWaylandLog(
    const std::string& filename
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open Wayland log: " + filename
        );
    }

    const std::regex statsPattern(
        R"(^(\d{2}:\d{2}:\d{2}\.\d+)\s+stats\s+last\s+10\s+s:\s+(\d+)\s+frames\s+on\s+screen\s+\(([0-9.]+)\s+fps\)\s+\|\s+(\d+)\s+GPU\s+frames\s+from\s+games.*?(\d+)\s+windows\s+open)"
    );

    const std::regex perfPattern(
        R"(^(\d{2}:\d{2}:\d{2}\.\d+)\s+perf\s+last\s+10\s+s:.*?render_scene\s+([0-9.]+)/([0-9.]+)\s+ms.*?acquire\s+([0-9.]+)/([0-9.]+)\s+ms.*?present\s+([0-9.]+)/([0-9.]+)\s+ms\s+\((\d+)\).*?fence wait\s+([0-9.]+)/([0-9.]+)\s+ms\s+\((\d+),\s+(\d+)\s+GPU release waits\).*?release\s+([0-9.]+)/([0-9.]+)\s+ms\s+\((\d+),\s+(\d+)\s+held\))"
    );

    const std::regex lifecyclePattern(
        R"(^(\d{2}:\d{2}:\d{2}\.\d+)\s+gpu\s+screen surface (attached|gone))"
    );

    // Identify stats/perf records even when their full format is invalid.
    const std::regex statsRecordPrefix(
        R"(^\d{2}:\d{2}:\d{2}\.\d+\s+stats\s+last\s+10\s+s:)"
    );

    const std::regex perfRecordPrefix(
        R"(^\d{2}:\d{2}:\d{2}\.\d+\s+perf\s+last\s+10\s+s:)"
    );

    // Pair stats and perf records by second.
    std::map<std::string, WaylandLogSample> samplesBySecond;

    // Store lifecycle events independently.
    std::vector<SurfaceLifecycleEvent> lifecycleEvents;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        std::smatch match;

        // Parse surface lifecycle events independently.
        if (std::regex_search(line, match, lifecyclePattern)) {
            SurfaceLifecycleEvent event;

            event.timestamp = match[1].str();
            event.attached = (match[2].str() == "attached");

            lifecycleEvents.push_back(event);
        }

        // Parse stats records.
        if (std::regex_search(line, match, statsPattern)) {
            const std::string timestamp = match[1].str();
            const std::string key = timestamp.substr(0, 8);

            WaylandLogSample& sample = samplesBySecond[key];
            sample.timestamp = timestamp;

            try {
                sample.framesOnScreen = std::stoll(match[2].str());

                sample.displayedFps =
                    parseDoubleStrict(match[3].str());

                sample.gpuFramesFromGames = std::stoll(match[4].str());
                sample.windowsOpen = std::stoll(match[5].str());

            } catch (const std::exception& error) {
                throw std::runtime_error(
                    "Failed parsing stats record at line " +
                    std::to_string(lineNumber) + ": " +
                    error.what() + " | Record: " + line
                );
            }

            sample.hasStatsData = true;
        }

        // Parse performance records.
        else if (std::regex_search(line, match, perfPattern)) {
            const std::string timestamp = match[1].str();
            const std::string key = timestamp.substr(0, 8);

            WaylandLogSample& sample = samplesBySecond[key];

            // Preserve the stats timestamp when available.
            if (sample.timestamp.empty()) {
                sample.timestamp = timestamp;
            }

            try {
                sample.renderSceneAvgMs =
                    parseDoubleStrict(match[2].str());

                sample.renderSceneMaxMs =
                    parseDoubleStrict(match[3].str());

                sample.acquireAvgMs =
                    parseDoubleStrict(match[4].str());

                sample.acquireMaxMs =
                    parseDoubleStrict(match[5].str());

                sample.presentAvgMs =
                    parseDoubleStrict(match[6].str());

                sample.presentMaxMs =
                    parseDoubleStrict(match[7].str());

                sample.fenceWaitAvgMs =
                    parseDoubleStrict(match[9].str());

                sample.fenceWaitMaxMs =
                    parseDoubleStrict(match[10].str());

                sample.gpuReleaseWaits =
                    std::stoll(match[12].str());

                sample.releaseAvgMs =
                    parseDoubleStrict(match[13].str());

                sample.releaseMaxMs =
                    parseDoubleStrict(match[14].str());

                sample.releaseFrameCount =
                    std::stoll(match[15].str());

                sample.releaseHeldFrames =
                    std::stoll(match[16].str());

            } catch (const std::exception& error) {
                throw std::runtime_error(
                    "Failed parsing perf record at line " +
                    std::to_string(lineNumber) + ": " +
                    error.what() + " | Record: " + line
                );
            }

            sample.hasPerfData = true;
        }

        // Reject malformed stats records.
        else if (std::regex_search(line, statsRecordPrefix)) {
            throw std::runtime_error(
                "Malformed stats record at line " +
                std::to_string(lineNumber) + ": " + line
            );
        }

        // Reject malformed performance records.
        else if (std::regex_search(line, perfRecordPrefix)) {
            throw std::runtime_error(
                "Malformed perf record at line " +
                std::to_string(lineNumber) + ": " + line
            );
        }
    }

    // Sort lifecycle events chronologically.
    std::sort(
        lifecycleEvents.begin(),
        lifecycleEvents.end(),
        [](const SurfaceLifecycleEvent& a,
           const SurfaceLifecycleEvent& b) {
            return a.timestamp < b.timestamp;
        }
    );

    // Return stats intervals in chronological order.
    std::vector<WaylandLogSample> samples;
    samples.reserve(samplesBySecond.size());

    for (const auto& entry : samplesBySecond) {
        const WaylandLogSample& sample = entry.second;

        if (sample.hasStatsData) {
            samples.push_back(sample);
        }
    }

    // Apply the latest lifecycle event preceding each sample.
    bool surfaceActive = false;
    std::size_t eventIndex = 0;

    for (auto& sample : samples) {
        while (eventIndex < lifecycleEvents.size() &&
               lifecycleEvents[eventIndex].timestamp <= sample.timestamp) {

            surfaceActive = lifecycleEvents[eventIndex].attached;
            ++eventIndex;
        }

        sample.surfaceActive = surfaceActive;
    }

    return samples;
}

#endif
