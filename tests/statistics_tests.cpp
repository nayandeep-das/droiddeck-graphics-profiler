
#include "statistics.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

bool approximatelyEqual(double actual, double expected) {
    return std::abs(actual - expected) < 0.001;
}

int main() {
    int failures = 0;

    // Test 1: Known frame-time dataset.
    {
        const std::vector<double> frameTimes = {
            25.0, 25.0, 50.0, 100.0
        };

        const FrameStatistics stats =
            calculateStatistics(frameTimes);

        if (!approximatelyEqual(stats.averageFrameTime, 50.0) ||
            !approximatelyEqual(stats.maximumFrameTime, 100.0) ||
            !approximatelyEqual(stats.averageFps, 20.0) ||
            stats.stutterCount != 1) {
            std::cerr << "FAIL: Basic statistics test\n";
            ++failures;
        } else {
            std::cout << "PASS: Basic statistics test\n";
        }
    }

    // Test 2: Empty input should throw an exception.
    {
        try {
            calculateStatistics({});
            std::cerr << "FAIL: Empty input test\n";
            ++failures;
        } catch (const std::invalid_argument&) {
            std::cout << "PASS: Empty input test\n";
        }
    }

    // Test 3: Zero threshold should be rejected.
    {
        try {
            calculateStatistics({25.0, 30.0}, 0.0);
            std::cerr << "FAIL: Invalid threshold test\n";
            ++failures;
        } catch (const std::invalid_argument&) {
            std::cout << "PASS: Invalid threshold test\n";
        }
    }

    std::cout << "\nFailed tests: " << failures << '\n';

    return failures == 0 ? 0 : 1;
}
