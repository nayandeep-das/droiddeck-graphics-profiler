# DroidDeck Graphics Profiler

A C++17 performance-analysis and diagnostic tool developed to investigate frame-time instability and stuttering in Android-based PC game emulation using DroidDeck.

The project analyzes frame-time data and Wayland/Gamescope telemetry to identify performance anomalies, compare gameplay conditions, investigate potential bottlenecks, and generate structured diagnostic reports.

## Features

### Frame-Time Analysis
- Read frame-time measurements from CSV files.
- Validate input and skip malformed rows.
- Calculate average and maximum frame time.
- Estimate average FPS.
- Detect stutters using a configurable frame-time threshold.

### Wayland / Gamescope Telemetry
- Parse Wayland/Gamescope log records.
- Correlate statistics and performance records by timestamp.
- Track presentation-surface lifecycle events.
- Extract FPS, frame counts, and graphics timing measurements.
- Handle incomplete and malformed telemetry records.

### Stutter and Anomaly Detection
- Detect critical and low-FPS intervals.
- Identify timing anomalies across rendering and presentation-related measurements.
- Classify events by severity.
- Filter inactive or non-gameplay samples.

### Diagnostic Analysis
- Compare critical-FPS and higher-FPS gameplay intervals.
- Calculate Pearson correlations between FPS and telemetry metrics.
- Generate evidence-based diagnostic hypotheses.
- Assign heuristic confidence levels.
- Produce investigation recommendations and validation experiments.

### Automated Reporting
- Generate Markdown diagnostic reports.
- Summarize performance statistics and anomaly counts.
- Include FPS group comparisons and baseline metrics.
- Document correlations, hypotheses, and recommendations.
- List detected stutter events with relevant timing information.

## Project Architecture

```text
droiddeck-graphics-profiler/
├── CMakeLists.txt
├── README.md
├── data/
│   ├── frame_times.csv
│   ├── wayland.log
│   └── droiddeck_wayland_10s_summary.csv
├── include/
│   ├── csv_reader.hpp
│   ├── statistics.hpp
│   ├── wayland_log_parser.h
│   ├── wayland_parser.h
│   ├── wayland_sample.h
│   ├── stutter_analyzer.hpp
│   ├── diagnostic_engine.hpp
│   ├── baseline_comparison.hpp
│   ├── correlation_analyzer.hpp
│   ├── hypothesis_engine.hpp
│   ├── recommendation_engine.hpp
│   └── report_generator.hpp
├── src/
│   ├── main.cpp
│   ├── csv_reader.cpp
│   ├── statistics.cpp
│   ├── stutter_analyzer.cpp
│   ├── diagnostic_engine.cpp
│   ├── baseline_comparison.cpp
│   ├── correlation_analyzer.cpp
│   ├── hypothesis_engine.cpp
│   ├── recommendation_engine.cpp
│   └── report_generator.cpp
├── tests/
└── reports/
```

## Requirements

- C++17-compatible compiler
- CMake 3.20 or newer
- Ninja build system

## Build

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

## Usage

### Frame-Time Analysis

Run the standard frame-time analyzer:

```powershell
.\build\frame_analyzer.exe
```

Specify a custom stutter threshold in milliseconds:

```powershell
.\build\frame_analyzer.exe --threshold 75
```

### Wayland / Gamescope Analysis

Run the telemetry-based diagnostic workflow:

```powershell
.\build\frame_analyzer.exe --wayland
```

This workflow parses the Wayland telemetry, detects anomalies, performs diagnostic analysis, and generates a Markdown report.

Generated report:

```text
reports/wayland_diagnostic_report.md
```

## Input Data

### Frame-Time CSV

The frame-time analyzer expects:

`data/frame_times.csv`

Example:

```csv
frame,time_ms
1,25.0
2,24.8
3,26.1
```

Frame times must be positive values in milliseconds.

### Wayland Telemetry

The Wayland diagnostic workflow uses the supplied telemetry log:

`data/wayland.log`

The parser extracts available performance statistics, timing measurements, and presentation lifecycle information.

## Testing

The project uses CTest for automated testing.

Run all tests:

```powershell
ctest --test-dir build --output-on-failure
```

The test suite covers:

- Statistics and frame-time calculations
- Wayland CSV parsing
- Wayland log parsing
- Stutter detection
- Diagnostic classification
- Baseline comparison
- Correlation analysis
- Hypothesis generation
- Recommendation generation
- Markdown report generation

**Current validation: 10/10 test suites passing.**

## Diagnostic Findings

In the analyzed telemetry session:

- 88 Wayland samples were loaded.
- 85 gameplay samples were analyzed.
- 67 stutter events were detected.
- 5 samples fell below 10 FPS.
- Average FPS in the critical group was 4.04, compared with 36.36 in the higher-FPS group.

The analysis identified presentation lifecycle activity as a possible investigation area. However, the available measurements do not establish a definitive root cause.

## Limitations

- The analysis is based on the supplied telemetry session.
- Ten-second aggregated measurements can hide short-lived events.
- The tool does not directly instrument the GPU or graphics driver.
- Correlation does not establish causation.
- Some frame-count and FPS measurements may be coupled by their measurement windows.
- Hypothesis confidence scores are heuristic rankings, not statistical probabilities.
- Additional gameplay sessions and independent per-frame measurements are required to establish root causes.

## Project Goal

Build a reproducible performance-analysis workflow for investigating frame-time spikes and stuttering in DroidDeck.

The project emphasizes measured telemetry, statistical analysis, diagnostic reasoning, controlled experiments, automated reporting, and transparent documentation of limitations.