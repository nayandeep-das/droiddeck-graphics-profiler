
#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <string>
#include <vector>

bool loadFrameTimes(
    const std::string& filePath,
    std::vector<double>& frameTimes
);

#endif
