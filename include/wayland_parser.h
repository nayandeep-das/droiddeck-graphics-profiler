
#ifndef WAYLAND_PARSER_H
#define WAYLAND_PARSER_H

#include "wayland_sample.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

inline std::optional<double> parseOptionalDouble(
    const std::string& value
) {
    if (value.empty()) {
        return std::nullopt;
    }

    try {
        std::size_t parsed = 0;
        double number = std::stod(value, &parsed);

        if (parsed != value.size()) {
            return std::nullopt;
        }

        return number;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

inline std::vector<WaylandSample> loadWaylandCsv(
    const std::string& filename
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open CSV file: " + filename
        );
    }

    std::string line;

    if (!std::getline(file, line)) {
        throw std::runtime_error("CSV file is empty");
    }

    std::vector<std::string> headers;
    {
        std::stringstream stream(line);
        std::string column;

        while (std::getline(stream, column, ',')) {
            headers.push_back(column);
        }
    }

    auto columnIndex = [&headers](const std::string& name) {
        for (std::size_t i = 0; i < headers.size(); ++i) {
            if (headers[i] == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    const int timestampCol = columnIndex("timestamp");
    const int framesCol = columnIndex("frames_on_screen");
    const int fpsCol = columnIndex("displayed_fps");
    const int gpuFramesCol = columnIndex("gpu_frames_from_games");
    const int phaseCol = columnIndex("phase_note");

    
    const int renderSceneAvgCol = columnIndex("render_scene_avg_ms");
    const int renderSceneMaxCol = columnIndex("render_scene_max_ms");

    const int acquireAvgCol = columnIndex("acquire_avg_ms");
    const int acquireMaxCol = columnIndex("acquire_max_ms");

    const int presentAvgCol = columnIndex("present_avg_ms");
    const int presentMaxCol = columnIndex("present_max_ms");

    const int fenceWaitAvgCol = columnIndex("fence_wait_avg_ms");
    const int fenceWaitMaxCol = columnIndex("fence_wait_max_ms");

    const int releaseAvgCol = columnIndex("release_avg_ms");
    const int releaseMaxCol = columnIndex("release_max_ms");


    if (timestampCol < 0 || framesCol < 0 ||
        fpsCol < 0 || gpuFramesCol < 0) {
        throw std::runtime_error(
            "CSV is missing one or more required columns"
        );
    }

    auto getCell = [](const std::vector<std::string>& cells,
                      int index) -> std::string {
        if (index < 0 ||
            static_cast<std::size_t>(index) >= cells.size()) {
            return "";
        }
        return cells[index];
    };

    std::vector<WaylandSample> samples;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> cells;
        std::stringstream stream(line);
        std::string cell;

        while (std::getline(stream, cell, ',')) {
            cells.push_back(cell);
        }

        WaylandSample sample;
        sample.timestamp = getCell(cells, timestampCol);
        sample.phaseNote = getCell(cells, phaseCol);

        
        sample.renderSceneAvgMs =
            parseOptionalDouble(getCell(cells, renderSceneAvgCol));
        sample.renderSceneMaxMs =
            parseOptionalDouble(getCell(cells, renderSceneMaxCol));

        sample.acquireAvgMs =
            parseOptionalDouble(getCell(cells, acquireAvgCol));
        sample.acquireMaxMs =
                parseOptionalDouble(getCell(cells, acquireMaxCol));

        sample.presentAvgMs =
            parseOptionalDouble(getCell(cells, presentAvgCol));
        sample.presentMaxMs =
            parseOptionalDouble(getCell(cells, presentMaxCol));

        sample.fenceWaitAvgMs =
            parseOptionalDouble(getCell(cells, fenceWaitAvgCol));
        sample.fenceWaitMaxMs =
            parseOptionalDouble(getCell(cells, fenceWaitMaxCol));

        sample.releaseAvgMs =
            parseOptionalDouble(getCell(cells, releaseAvgCol));
        sample.releaseMaxMs =
            parseOptionalDouble(getCell(cells, releaseMaxCol));
            
        try {
            const std::string framesText = getCell(cells, framesCol);
            const std::string fpsText = getCell(cells, fpsCol);
            const std::string gpuFramesText = getCell(cells, gpuFramesCol);

            std::size_t framesParsed = 0;
            std::size_t fpsParsed = 0;
            std::size_t gpuFramesParsed = 0;

            sample.framesOnScreen = std::stoll(framesText, &framesParsed);
            sample.displayedFps = std::stod(fpsText, &fpsParsed);
            sample.gpuFramesFromGames = std::stoll(gpuFramesText, &gpuFramesParsed);

            if (framesParsed != framesText.size() ||
                fpsParsed != fpsText.size() ||
                gpuFramesParsed != gpuFramesText.size() ||
                sample.framesOnScreen < 0 ||
                sample.displayedFps < 0.0 ||
                sample.gpuFramesFromGames < 0) {
                continue;
            }

        } catch (const std::exception&) {
            continue;
        }


        samples.push_back(sample);
    }

    return samples;
}

#endif
