
#ifndef WAYLAND_SAMPLE_H
#define WAYLAND_SAMPLE_H

#include <optional>
#include <string>

struct WaylandSample {
    std::string timestamp;
    std::string phaseNote;

    long long framesOnScreen = 0;
    double displayedFps = 0.0;
    long long gpuFramesFromGames = 0;

    // Empty CSV cells remain distinguishable from actual zero values.
    std::optional<double> renderSceneAvgMs;
    std::optional<double> renderSceneMaxMs;

    std::optional<double> acquireAvgMs;
    std::optional<double> acquireMaxMs;

    std::optional<double> presentAvgMs;
    std::optional<double> presentMaxMs;

    std::optional<double> fenceWaitAvgMs;
    std::optional<double> fenceWaitMaxMs;

    std::optional<double> releaseAvgMs;
    std::optional<double> releaseMaxMs;
};

#endif
