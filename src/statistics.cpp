
#include "statistics.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

FrameStatistics calculateStatistics(
    const std::vector<double>& frameTimes,
    double stutterThresholdMs
) {
    if (frameTimes.empty()) {
        throw std::invalid_argument(
            "Cannot calculate statistics for empty frame-time data."
        );
    }

    if (stutterThresholdMs <= 0.0) {
        throw std::invalid_argument(
            "Stutter threshold must be positive."
        );
    }

    const double totalFrameTime =
        std::accumulate(
            frameTimes.begin(),
            frameTimes.end(),
            0.0
        );

    const double averageFrameTime =
        totalFrameTime / frameTimes.size();

    const double maximumFrameTime =
        *std::max_element(
            frameTimes.begin(),
            frameTimes.end()
        );

    const double averageFps = 1000.0 / averageFrameTime;

    int stutterCount = 0;

    for (double frameTime : frameTimes) {
        if (frameTime > stutterThresholdMs) {
            ++stutterCount;
        }
    }

    return {
        averageFrameTime,
        maximumFrameTime,
        averageFps,
        stutterCount
    };
}
