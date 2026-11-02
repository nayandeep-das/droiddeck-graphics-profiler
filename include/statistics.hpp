
#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <vector>

struct FrameStatistics {
    double averageFrameTime;
    double maximumFrameTime;
    double averageFps;
    int stutterCount;
};

FrameStatistics calculateStatistics(
    const std::vector<double>& frameTimes,
    double stutterThresholdMs = 50.0
);

#endif
