
cmake_minimum_required(VERSION 3.20)

project(DroidDeckGraphicsProfiler
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(frame_analyzer
    src/main.cpp
    src/statistics.cpp
    src/csv_reader.cpp
    src/stutter_analyzer.cpp
    src/diagnostic_engine.cpp
    src/baseline_comparison.cpp
    src/correlation_analyzer.cpp
    src/hypothesis_engine.cpp
    src/recommendation_engine.cpp
    src/report_generator.cpp
)

target_include_directories(frame_analyzer PRIVATE
    include
)

target_compile_options(frame_analyzer PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

enable_testing()

add_executable(statistics_tests
    tests/statistics_tests.cpp
    src/statistics.cpp
)

target_include_directories(statistics_tests PRIVATE
    include
)

target_compile_options(statistics_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME StatisticsTests
    COMMAND statistics_tests
)

add_executable(wayland_parser_test
    tests/wayland_parser_test.cpp
)

target_include_directories(wayland_parser_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(wayland_parser_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME WaylandParserTest
    COMMAND wayland_parser_test
)

set_tests_properties(WaylandParserTest PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(wayland_log_parser_test
    tests/wayland_log_parser_test.cpp
)

target_include_directories(wayland_log_parser_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(wayland_log_parser_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME WaylandLogParserTest
    COMMAND wayland_log_parser_test
)

set_tests_properties(WaylandLogParserTest PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(stutter_analyzer_tests
    tests/stutter_analyzer_tests.cpp
    src/stutter_analyzer.cpp
)

target_include_directories(stutter_analyzer_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(stutter_analyzer_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME StutterAnalyzerTests
    COMMAND stutter_analyzer_tests
)

set_tests_properties(StutterAnalyzerTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(diagnostic_engine_tests
    tests/diagnostic_engine_tests.cpp
    src/diagnostic_engine.cpp
    src/stutter_analyzer.cpp
)

target_include_directories(diagnostic_engine_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(diagnostic_engine_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME DiagnosticEngineTests
    COMMAND diagnostic_engine_tests
)

set_tests_properties(DiagnosticEngineTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(baseline_comparison_tests
    tests/baseline_comparison_tests.cpp
    src/baseline_comparison.cpp
)

target_include_directories(baseline_comparison_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(baseline_comparison_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME BaselineComparisonTests
    COMMAND baseline_comparison_tests
)

set_tests_properties(BaselineComparisonTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(correlation_analyzer_tests
    tests/correlation_analyzer_tests.cpp
    src/correlation_analyzer.cpp
)

target_include_directories(correlation_analyzer_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(correlation_analyzer_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME CorrelationAnalyzerTests
    COMMAND correlation_analyzer_tests
)

set_tests_properties(CorrelationAnalyzerTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Diagnostic Hypothesis Engine Tests

add_executable(hypothesis_engine_tests
    tests/hypothesis_engine_tests.cpp
    src/hypothesis_engine.cpp
)

target_include_directories(hypothesis_engine_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(hypothesis_engine_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME HypothesisEngineTests
    COMMAND hypothesis_engine_tests
)

set_tests_properties(HypothesisEngineTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(recommendation_engine_tests
    tests/recommendation_engine_tests.cpp
    src/recommendation_engine.cpp
    src/hypothesis_engine.cpp
)

target_include_directories(recommendation_engine_tests PRIVATE include)

add_test(
    NAME RecommendationEngineTests
    COMMAND recommendation_engine_tests
)

add_executable(report_generator_tests
    tests/report_generator_tests.cpp
    src/report_generator.cpp
    src/hypothesis_engine.cpp
    src/recommendation_engine.cpp
    src/stutter_analyzer.cpp
)

target_include_directories(report_generator_tests PRIVATE include)

add_test(
    NAME ReportGeneratorTests
    COMMAND report_generator_tests
)