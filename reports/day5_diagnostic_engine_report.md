# Day 5 --- Diagnostic Engine and Evidence Enrichment

**Project:** DroidDeck Graphics Profiler\
**Date:** November 2, 2026\
**Status:** Complete --- all automated test suites passing

## 1. Objective

Develop a diagnostic layer that converts detected Wayland/Gamescope
stutter events into structured, evidence-backed diagnostic results. The
goal is to move beyond raw telemetry and prepare the profiler to explain
observed performance anomalies without claiming unsupported root causes.

## 2. Work Completed

### 2.1 Diagnostic classification

Added a diagnostic engine with three categories:

-   `FPS_DEGRADATION` --- FPS degradation was detected without a
    monitored subsystem timing anomaly.
-   `TIMING_ANOMALY` --- one or more monitored subsystem timing
    thresholds were exceeded without detected FPS degradation.
-   `COMBINED_ANOMALY` --- FPS degradation and timing anomalies occurred
    in the same sample.

Events with neither `fpsDegradation` nor `timingAnomaly` set are
ignored. Empty input produces an empty result.

### 2.2 Structured diagnostic results

Added `DiagnosticResult` to represent each diagnostic with:

-   Timestamp
-   Diagnostic category
-   Severity
-   Displayed FPS
-   Evidence strings
-   Evidence-based interpretation

The result also preserves the maximum timing measurements from the
source `StutterEvent`:

-   Render Scene maximum (ms)
-   Present maximum (ms)
-   Fence Wait maximum (ms)
-   Release maximum (ms)

### 2.3 Evidence enrichment

The diagnostic engine preserves the original detection reasons and adds
formatted evidence for displayed FPS and all four subsystem maximum
timings. Timing values are formatted to two decimal places and include
millisecond units.

### 2.4 Evidence-based interpretation

Each category receives an interpretation that describes what the
telemetry establishes. For combined anomalies, the interpretation
explicitly distinguishes temporal correlation from causation. The
diagnostic engine does not claim that a subsystem timing anomaly caused
an FPS drop.

## 3. Tests and Validation

Expanded `DiagnosticEngineTests` to cover:

1.  FPS degradation-only classification.
2.  Timing anomaly-only classification.
3.  Combined anomaly classification.
4.  Preservation of timestamp, FPS, severity and original evidence.
5.  Preservation of all four subsystem maximum timing values.
6.  Inclusion of correctly formatted subsystem timing evidence.
7.  Empty input handling.
8.  Ignoring events with no detected condition.

The project was rebuilt and the complete CTest suite was run
successfully.

  Test suite                Result
  ------------------------- --------
  `StatisticsTests`         Passed
  `WaylandParserTest`       Passed
  `WaylandLogParserTest`    Passed
  `StutterAnalyzerTests`    Passed
  `DiagnosticEngineTests`   Passed

**Validation result: 5/5 test suites passed.**

Commands used:

``` powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

## 4. Current Diagnostic Capabilities

The profiler can now:

-   Parse the existing Wayland/Gamescope telemetry through the
    established parser.
-   Detect FPS degradation and monitored subsystem timing anomalies.
-   Classify detected events into structured diagnostic categories.
-   Preserve relevant timing measurements and detection evidence.
-   Generate cautious interpretations that do not overstate causality.

## 5. Current Limitations

The diagnostic engine currently classifies and explains detected events;
it does **not** yet identify a confirmed root cause or automatically
determine the best settings to improve FPS.

In particular:

-   A timing threshold exceedance is an anomaly indicator, not proof of
    a bottleneck.
-   Low FPS without a monitored timing anomaly may involve CPU
    scheduling, GPU workload, synchronization, missing telemetry or
    other unmeasured factors.
-   Recommendations such as lowering resolution must be supported by
    controlled comparisons, not inferred from low FPS alone.
-   Current evidence is limited to the available telemetry fields and
    configured thresholds.

## 6. Next Steps

### Day 6 --- Robustness and validation

-   Test malformed log records.
-   Validate handling of incomplete telemetry and missing stats/perf
    pairs.
-   Exercise timestamp ordering and lifecycle edge cases.
-   Test threshold validation and error handling.
-   Run the full regression suite.

### Day 7 --- Diagnostic reasoning

Extend the diagnostic layer to produce evidence-backed hypotheses,
confidence/limitations, and actionable experiments. Compare low-FPS
intervals against stable intervals before ranking possible bottlenecks.

### Day 8 --- Reporting and finalization

Produce a readable performance diagnostic report, document the
architecture and methodology, state limitations, and prepare a
reproducible demonstration.

## 7. Conclusion

Day 5 completed the first structured diagnostic layer of the DroidDeck
Graphics Profiler. The project now converts detected stutter events into
categorized results while retaining the original telemetry evidence and
subsystem timing measurements. All five test suites pass.

The next major milestone is to turn these structured observations into
useful performance investigations and recommendations. Root-cause claims
will remain evidence-dependent and will require controlled validation.
