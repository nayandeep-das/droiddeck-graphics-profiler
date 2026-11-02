
#include "wayland_parser.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

int main() {
    try {
        const auto samples = loadWaylandCsv(
            "data/droiddeck_wayland_10s_summary.csv"
        );

        std::cout << "Samples loaded: "
                  << samples.size() << '\n';

        if (samples.empty()) {
            std::cerr << "No valid samples found.\n";
            return 1;
        }

        // Count available timing measurements.
        std::size_t renderTimingCount = 0;
        std::size_t acquireTimingCount = 0;
        std::size_t presentTimingCount = 0;
        std::size_t fenceWaitTimingCount = 0;
        std::size_t releaseTimingCount = 0;

        for (const auto& sample : samples) {
            if (sample.renderSceneAvgMs) ++renderTimingCount;
            if (sample.acquireAvgMs) ++acquireTimingCount;
            if (sample.presentAvgMs) ++presentTimingCount;
            if (sample.fenceWaitAvgMs) ++fenceWaitTimingCount;
            if (sample.releaseAvgMs) ++releaseTimingCount;
        }

        std::cout << "\nTiming data availability:\n";
        std::cout << "Render-scene: " << renderTimingCount << '\n';
        std::cout << "Acquire: " << acquireTimingCount << '\n';
        std::cout << "Present: " << presentTimingCount << '\n';
        std::cout << "Fence wait: " << fenceWaitTimingCount << '\n';
        std::cout << "Release: " << releaseTimingCount << '\n';

        // Verify a known observation.
        const auto knownSample = std::find_if(
            samples.begin(), samples.end(),
            [](const WaylandSample& sample) {
                return sample.timestamp == "23:15.6";
            }
        );

        if (knownSample == samples.end() ||
            !knownSample->renderSceneAvgMs ||
            !knownSample->renderSceneMaxMs) {
            std::cerr << "Known timing verification failed.\n";
            return 1;
        }

        std::cout << "\nTiming verification for "
                  << knownSample->timestamp << ":\n";
        std::cout << "Render-scene average: "
                  << *knownSample->renderSceneAvgMs << " ms\n";
        std::cout << "Render-scene maximum: "
                  << *knownSample->renderSceneMaxMs << " ms\n";

        // Overall FPS range.
        const auto lowest = std::min_element(
            samples.begin(), samples.end(),
            [](const WaylandSample& a, const WaylandSample& b) {
                return a.displayedFps < b.displayedFps;
            }
        );

        const auto highest = std::max_element(
            samples.begin(), samples.end(),
            [](const WaylandSample& a, const WaylandSample& b) {
                return a.displayedFps < b.displayedFps;
            }
        );

        std::cout << "\nOverall FPS range:\n";
        std::cout << "Lowest: " << lowest->displayedFps
                  << " at " << lowest->timestamp << '\n';
        std::cout << "Highest: " << highest->displayedFps
                  << " at " << highest->timestamp << '\n';

        // Restrict the diagnostic groups to Eastward-active intervals.
        const auto isEastwardActive = [](const WaylandSample& sample) {
            return sample.phaseNote.find("Eastward window active")
                   != std::string::npos;
        };

        const auto isLowFps = [&](const WaylandSample& sample) {
            return isEastwardActive(sample)
                && sample.displayedFps <= 5.0;
        };

        const auto isHealthyFps = [&](const WaylandSample& sample) {
            return isEastwardActive(sample)
                && sample.displayedFps >= 30.0;
        };

        std::size_t lowCount = 0;
        std::size_t healthyCount = 0;

        std::cout << "\nEastward-active intervals at or below 5 FPS:\n";
        for (const auto& sample : samples) {
            if (isLowFps(sample)) {
                ++lowCount;
                std::cout << "Timestamp: " << sample.timestamp
                          << " | FPS: " << sample.displayedFps
                          << " | Frames: " << sample.framesOnScreen
                          << " | GPU frames: "
                          << sample.gpuFramesFromGames << '\n';
            }
        }

        for (const auto& sample : samples) {
            if (isHealthyFps(sample)) {
                ++healthyCount;
            }
        }

        std::cout << "\nComparison group sizes:\n";
        std::cout << "Low-FPS intervals (<= 5 FPS): "
                  << lowCount << '\n';
        std::cout << "Healthy-FPS intervals (>= 30 FPS): "
                  << healthyCount << '\n';

        // Compare the available aggregate timing values.
        // Each metric reports its own sample count because fields
        // may be missing in some observations.
        std::cout << "\nTiming comparison (milliseconds):\n";
        std::cout << "Values are means across available 10-second "
                     "aggregate observations, not per-frame timings.\n";

        const auto printMetric = [&](const std::string& name,
                                     const auto& getter) {
            const auto printGroup = [&](const std::string& groupName,
                                        const auto& predicate) {
                double sum = 0.0;
                std::size_t count = 0;

                for (const auto& sample : samples) {
                    if (!predicate(sample)) {
                        continue;
                    }

                    const auto value = getter(sample);
                    if (value.has_value()) {
                        sum += *value;
                        ++count;
                    }
                }

                std::cout << "  " << groupName << ": ";
                if (count == 0) {
                    std::cout << "N/A (0 samples)\n";
                } else {
                    std::cout << std::fixed << std::setprecision(2)
                              << (sum / static_cast<double>(count))
                              << " ms (" << count << " samples)\n";
                }
            };

            std::cout << "\n" << name << '\n';
            printGroup("Low FPS", isLowFps);
            printGroup("Healthy FPS", isHealthyFps);
        };

        printMetric("Render-scene average", [](const WaylandSample& s) {
            return s.renderSceneAvgMs;
        });
        printMetric("Render-scene maximum", [](const WaylandSample& s) {
            return s.renderSceneMaxMs;
        });
        printMetric("Acquire average", [](const WaylandSample& s) {
            return s.acquireAvgMs;
        });
        printMetric("Acquire maximum", [](const WaylandSample& s) {
            return s.acquireMaxMs;
        });
        printMetric("Present average", [](const WaylandSample& s) {
            return s.presentAvgMs;
        });
        printMetric("Present maximum", [](const WaylandSample& s) {
            return s.presentMaxMs;
        });
        printMetric("Fence-wait average", [](const WaylandSample& s) {
            return s.fenceWaitAvgMs;
        });
        printMetric("Fence-wait maximum", [](const WaylandSample& s) {
            return s.fenceWaitMaxMs;
        });
        printMetric("Release average", [](const WaylandSample& s) {
            return s.releaseAvgMs;
        });
        printMetric("Release maximum", [](const WaylandSample& s) {
            return s.releaseMaxMs;
        });

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
