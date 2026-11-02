# Day 6 --- Robustness and Validation

**Project:** DroidDeck Graphics Profiler\
**Date:** November 2, 2026\
**Status:** Complete --- all automated test suites passing

## 1. Objective

Improve the reliability of the Wayland telemetry ingestion pipeline when
handling malformed records, incomplete telemetry, irregular event
ordering and invalid numeric values. Preserve existing functionality and
ensure the profiler continues to process the real benchmark log
correctly.

## 2. Work Completed

### 2.1 Strict numeric validation

Introduced `parseDoubleStrict()` to ensure floating-point values are
fully consumed and finite.

The parser now rejects: - Malformed values such as `12.3.4` - `NaN` -
Positive and negative infinity - Values containing trailing invalid
characters

Valid floating-point values continue to be accepted.

### 2.2 Malformed telemetry detection

Added explicit detection for malformed `stats` and `perf` summary
records.

Improvements: - Malformed telemetry is rejected instead of silently
ignored. - Error messages include the offending input line number. -
Summary-record detection ignores unrelated compositor messages.

A false positive caused by matching unrelated `perf` messages was
identified and fixed by narrowing the record-prefix regular expression
to the actual summary format.

### 2.3 Incomplete telemetry handling

Verified the parser's behaviour for unmatched records.

  -----------------------------------------------------------------------
  Input condition                     Parser behaviour
  ----------------------------------- -----------------------------------
  Stats and perf records available    Combined into one sample

  Stats record without perf record    Retained as a stats-only sample

  Perf record without stats record    Excluded from returned samples

  Missing timing metrics              Availability flags distinguish
                                      missing data from valid telemetry
  -----------------------------------------------------------------------

The `hasStatsData` and `hasPerfData` flags preserve the distinction
between available and missing telemetry.

### 2.4 Timestamp and lifecycle validation

Added regression coverage for lifecycle events that appear out of
chronological order in the input log.

Verified: - Lifecycle events are sorted chronologically. - Returned
samples are chronological. - Surface attachment activates the GPU
surface. - Surface detachment deactivates it at the correct timestamp.

### 2.5 Numerical and threshold validation

Added direct tests for invalid floating-point values and valid numeric
input.

Cases tested include `12.3.4`, `nan`, `inf`, `-inf`, and a valid value
such as `12.5`. Existing stutter-analyzer threshold-validation tests
were retained.

## 3. Tests and Validation

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

**Final CTest result:** 100% tests passed, 0 tests failed out of 5.\
**Total test time:** 0.15 seconds.

Commands used:

``` powershell
cmake --build build
ctest --test-dir build --output-on-failure
.\build\frame_analyzer.exe --wayland
```

## 4. Real-World Integration Results

The final executable was run against the existing `data/wayland.log`.

  Metric                        Result
  --------------------------- --------
  Wayland samples loaded            88
  Gameplay samples analyzed         85
  Stutter events detected           67
  FPS degradation events             7
  Timing anomaly events             61
  Combined events                    1

### Subsystem timing anomalies

  Subsystem        Count
  -------------- -------
  Render Scene         4
  Present              0
  Fence Wait           2
  Release             59

The final integration output matched the previously established
baseline.

## 5. Performance Observations

-   Critical-FPS samples averaged **4.04 FPS**, while their average
    Render Scene time was **5.64 ms**.
-   Higher-FPS samples averaged **36.36 FPS**, while their average
    Render Scene time was **6.66 ms**.
-   Release timing anomalies accounted for **59 of the 67** detected
    stutter events.
-   GPU release waits were zero across all four FPS groups.

**Interpretation:** Render Scene timing alone does not explain the
observed FPS degradation. Release timing warrants further investigation,
but these measurements do not prove that it is the root cause.

## 6. Current Diagnostic and Robustness Capabilities

The profiler can now parse the existing Wayland/Gamescope telemetry,
reject malformed summary records and invalid numeric values, retain
stats-only samples, exclude perf-only samples from returned gameplay
samples, and apply lifecycle events chronologically.

It can also detect FPS degradation and monitored subsystem timing
anomalies, classify detected events into structured diagnostic
categories, and preserve relevant timing measurements and detection
evidence.

## 7. Current Limitations

Day 6 focused on robustness and validation; it did not establish a
confirmed performance root cause.

-   A timing threshold exceedance is an anomaly indicator, not proof of
    a bottleneck.
-   Low FPS without a monitored timing anomaly may involve CPU
    scheduling, GPU workload, synchronization, missing telemetry or
    other unmeasured factors.
-   Release timing anomalies are frequent in this log, but their
    relationship to FPS degradation has not been established.
-   Recommendations such as lowering resolution must be supported by
    controlled comparisons, not inferred from low FPS alone.
-   Current evidence is limited to the available telemetry fields and
    configured thresholds.

## 8. Next Steps

### Day 7 --- Diagnostic reasoning

-   Compare low-FPS intervals against stable gameplay intervals.
-   Identify correlated FPS and subsystem-timing behaviour.
-   Generate evidence-backed diagnostic hypotheses.
-   Include confidence levels and explicit limitations.
-   Recommend controlled experiments to investigate possible
    bottlenecks.

### Day 8 --- Reporting and finalization

-   Produce a readable performance diagnostic report.
-   Document the architecture and methodology.
-   State the limitations of the available telemetry.
-   Prepare a reproducible demonstration.

## 9. Conclusion

Day 6 strengthened the reliability of the DroidDeck Graphics Profiler's
telemetry ingestion and validation pipeline. The parser now rejects
malformed summary records and invalid numeric values, handles incomplete
telemetry explicitly, and correctly applies lifecycle events in
chronological order.

The complete regression suite passed, and the executable processed the
original Wayland benchmark log with all baseline counts preserved.

The next major milestone is to turn these structured observations into
useful performance investigations and recommendations. Root-cause claims
will remain evidence-dependent and require controlled validation.

**Day 6 status: Complete --- all five test suites passing.**
