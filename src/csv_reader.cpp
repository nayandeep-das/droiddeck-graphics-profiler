
#include "csv_reader.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

bool loadFrameTimes(
    const std::string& filePath,
    std::vector<double>& frameTimes
) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filePath << '\n';
        return false;
    }

    std::string line;

    // Skip the CSV header.
    if (!std::getline(file, line)) {
        std::cerr << "Error: CSV file is empty.\n";
        return false;
    }

    std::size_t lineNumber = 1;

    while (std::getline(file, line)) {
        ++lineNumber;

        if (line.empty()) {
            continue;
        }

        std::stringstream row(line);
        std::string frameNumber;
        std::string timeValue;

        if (!std::getline(row, frameNumber, ',') ||
            !std::getline(row, timeValue)) {
            std::cerr << "Warning: Skipping malformed row "
                      << lineNumber << ": " << line << '\n';
            continue;
        }

        try {
            std::size_t parsedCharacters = 0;
            const double frameTime =
                std::stod(timeValue, &parsedCharacters);

            if (parsedCharacters != timeValue.size() ||
                !std::isfinite(frameTime) ||
                frameTime <= 0.0 ||
                frameTime > 1e12) {
                std::cerr << "Warning: Skipping invalid frame time on row "
                          << lineNumber << ": " << timeValue << '\n';
                continue;
            }

            frameTimes.push_back(frameTime);
        } catch (const std::exception&) {
            std::cerr << "Warning: Skipping invalid frame time on row "
                      << lineNumber << ": " << timeValue << '\n';
        }
    }

    if (frameTimes.empty()) {
        std::cerr << "Error: No valid frame-time data found.\n";
        return false;
    }

    return true;
}
