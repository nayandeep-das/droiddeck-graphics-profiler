# DroidDeck Graphics Profiler

## Day 7 --- Diagnostic Reasoning Engine Report

**Project:** DroidDeck Graphics Profiler\
**Milestone:** Day 7 --- Diagnostic Reasoning Engine\
**Status:** Complete\
**Validation:** 9/9 test suites passed\
**Analysis mode:** Wayland / Gamescope telemetry (`--wayland`)

------------------------------------------------------------------------

## 1. Executive Summary

Day 7 extended the profiler from detecting performance anomalies to
comparing performance groups, examining relationships between telemetry
metrics, forming evidence-scored diagnostic hypotheses, and generating
practical investigation recommendations.

The implementation delivered four modules:

1.  Baseline Comparison Engine
2.  Correlation Analyzer
3.  Diagnostic Hypothesis Engine
4.  Actionable Recommendation Engine

The integrated program successfully analyzed 88 Wayland samples,
selected 85 gameplay samples for analysis, detected 67 stutter/timing
events, and generated five diagnostic hypotheses with corresponding
investigation recommendations.

All nine registered test suites passed after integration.

The analysis does **not** establish a definitive root cause for the
observed FPS degradation. In particular, the strongest frame-production
correlations may be coupled to displayed FPS and require independent
measurements.

## 2. Implemented Milestones

### 2.1 Baseline Comparison Engine

**Purpose:** Compare critical-FPS gameplay samples against higher-FPS
gameplay samples to identify telemetry differences.

The gameplay filter requires:

-   Statistics data to be present.
-   Performance data to be present.
-   At least one open window.
-   An active GPU presentation surface.
-   Non-zero GPU frames from games.

The analysis groups critical samples below 10 FPS and higher-FPS samples
at or above 30 FPS. It computes sample counts, FPS ranges and averages,
subsystem timing averages, and metric differences.

**Observed results:**

  Metric                   Critical group   Higher-FPS group
  ---------------------- ---------------- ------------------
  Samples                               5                 56
  Average FPS                        4.04              36.36
  Render Scene average            5.64 ms            6.66 ms
  Acquire average                 0.15 ms            0.19 ms
  Present average                 1.28 ms            1.35 ms
  Fence Wait average              3.43 ms            4.31 ms
  Release average                 5.17 ms           14.33 ms

The critical group has lower averages for all listed subsystem timing
metrics. These aggregate comparisons do not support identifying Render
Scene, Acquire, Present, Fence Wait, or Release average latency as the
direct cause of the critical FPS intervals.

The critical group contains only five samples, so the comparison should
be treated as preliminary.

### 2.2 Correlation Analyzer

**Purpose:** Calculate Pearson correlations between displayed FPS and
selected telemetry metrics, and compare critical and higher-FPS group
averages.

**Observed results:**

  Relationship                     Valid samples   Pearson r
  ------------------------------ --------------- -----------
  FPS vs GPU frames from games                85      0.9982
  FPS vs frames on screen                     85      1.0000
  FPS vs held-frame ratio                     85      0.7243
  FPS vs GPU release waits                    85         N/A

The release-wait correlation is undefined because the observed values
are constant at zero.

**Interpretation limitations:**

-   FPS and frames on screen are measured over the same ten-second
    window. Their near-perfect correlation is expected from the
    measurement relationship and is not independent evidence of a
    bottleneck.
-   GPU frames from games may also be coupled to displayed FPS. The
    correlation is strong, but it does not establish why frame
    production falls.
-   The held-frame ratio has a positive association with FPS in this
    dataset. This is an association, not proof of a causal mechanism.
-   Pearson correlation measures linear association and does not
    establish causation.

The next useful measurement is an independent application-side or
GPU-side frame-production timeline.

### 2.3 Diagnostic Hypothesis Engine

**Purpose:** Turn observed differences and temporal associations into
explicit hypotheses with supporting and contradictory evidence.

The engine evaluates five categories:

1.  Presentation Lifecycle Interference
2.  Frame Production Degradation
3.  Release Timing Bottleneck
4.  Render Bottleneck
5.  Synchronization Bottleneck

Evidence scores are heuristic rankings, not probabilities. Confidence
levels are assigned using evidence quantity and score thresholds.
Evidence items may not be independent.

**Observed hypothesis results:**

  Hypothesis                            Confidence     Evidence score
  ------------------------------------- ------------ ----------------
  Presentation Lifecycle Interference   Medium              44.44/100
  Frame Production Degradation          Low                 33.33/100
  Release Timing Bottleneck             Low                  0.00/100
  Render Bottleneck                     Low                  0.00/100
  Synchronization Bottleneck            Low                  0.00/100

#### Hypothesis 1 --- Presentation Lifecycle Interference

Supporting evidence:

-   1 of 5 critical samples occurred within 30 seconds of a surface
    lifecycle transition.

Contradictory evidence:

-   Some critical samples occurred outside the 30-second lifecycle
    window.

Interpretation: Lifecycle transitions are worth investigating, but they
do not explain every critical interval in the observed dataset.

#### Hypothesis 2 --- Frame Production Degradation

Supporting evidence:

-   Critical intervals averaged 69 GPU frames from games, compared with
    714.392857 in higher-FPS intervals.

Contradictory evidence:

-   No contradictory evidence was identified by the current heuristic.

Interpretation: Reduced frame production is consistent with the low-FPS
intervals. However, the GPU-frame count may be measurement-coupled to
displayed FPS, and the current data does not identify the underlying
reason.

#### Hypothesis 3 --- Release Timing Bottleneck

Contradictory evidence:

-   Critical release average: 5.168 ms.
-   Higher-FPS release average: 14.334643 ms.
-   Release maximum anomaly rates: critical 0.00%; higher-FPS
    82.142857%.

Interpretation: The current aggregate evidence does not support release
timing as the primary explanation for critical FPS degradation. Release
timing anomalies occur frequently in higher-FPS intervals.

#### Hypothesis 4 --- Render Bottleneck

Contradictory evidence:

-   Render Scene average: critical 5.64 ms; higher-FPS 6.661429 ms.
-   Render Scene maximum anomaly rates: critical 0.00%; higher-FPS
    3.571429%.

Interpretation: Current aggregate render timings do not show a clear
increase during critical intervals.

#### Hypothesis 5 --- Synchronization Bottleneck

Contradictory evidence:

-   Acquire average: critical 0.148 ms; higher-FPS 0.185179 ms.
-   Fence Wait average: critical 3.434 ms; higher-FPS 4.309464 ms.
-   Acquire/Fence maximum anomalies: critical 0; higher-FPS 1.

Interpretation: The current measurements do not support synchronization
as the primary explanation.

### 2.4 Actionable Recommendation Engine

**Purpose:** Convert hypotheses into investigation steps and validation
experiments without presenting hypotheses as confirmed root causes.

The engine generated five recommendations:

  Priority   Recommendation
  ---------- --------------------------------------
  Medium     Investigate Presentation Lifecycle
  Low        Investigate Frame Production
  Low        Investigate Buffer Release Behaviour
  Low        Investigate Rendering Workload
  Low        Investigate GPU Synchronization

Priority distribution:

-   High: 0
-   Medium: 1
-   Low: 4

The recommendations emphasize collecting independent or
higher-resolution measurements, examining individual stutter events,
correlating lifecycle events with FPS, and repeating tests under
controlled conditions.

No driver, graphics-setting, or emulator changes are prescribed as a
result of correlation alone.

## 3. Integrated Runtime Results

The final integrated `--wayland` execution reported:

  Runtime metric                    Result
  ------------------------------- --------
  Wayland samples loaded                88
  Stutter events detected               67
  Gameplay samples analyzed             85
  FPS degradation events                 7
  Timing anomaly events                 61
  Combined events                        1
  Render Scene timing anomalies          4
  Present timing anomalies               0
  Fence Wait timing anomalies            2
  Release timing anomalies              59

These are outputs from the current supplied telemetry dataset and
configured thresholds. They should not be interpreted as universal
performance characteristics.

## 4. Testing and Validation

The final build and test run completed successfully.

  Test suite                  Result
  --------------------------- --------
  StatisticsTests             Passed
  WaylandParserTest           Passed
  WaylandLogParserTest        Passed
  StutterAnalyzerTests        Passed
  DiagnosticEngineTests       Passed
  BaselineComparisonTests     Passed
  CorrelationAnalyzerTests    Passed
  HypothesisEngineTests       Passed
  RecommendationEngineTests   Passed

**Total: 9/9 test suites passed.**

The Recommendation Engine tests cover empty input, five-category
coverage, distinct hypothesis mapping, complete recommendation content,
priority assignment, and priority string conversion.

## 5. Key Engineering Decisions

-   Preserve the original frame-time analysis mode while extending the
    Wayland analysis mode.
-   Filter gameplay samples using telemetry availability, window count,
    surface lifecycle state, and GPU frame count.
-   Compare critical and higher-FPS groups rather than relying only on
    isolated maxima.
-   Treat undefined correlation results explicitly as N/A.
-   Keep evidence scores heuristic and confidence separate from causal
    certainty.
-   Generate investigation steps and experiments rather than
    automatically prescribing configuration changes.
-   Retain contradictory evidence in the report so that weak hypotheses
    are not overstated.

## 6. Limitations

1.  The critical comparison group contains only five samples.
2.  Several telemetry metrics are aggregated over ten-second windows,
    which can hide short-lived events and temporal ordering.
3.  FPS and frame-count measurements may be mathematically or
    operationally coupled.
4.  Correlation and temporal proximity do not establish causation.
5.  GPU release waits were constant at zero in the supplied samples,
    making their correlation undefined.
6.  The hypothesis evidence score is a project heuristic, not a
    statistical probability or calibrated confidence estimate.
7.  The current dataset represents the captured Eastward/DroidDeck
    session and does not establish general behavior across games,
    devices, drivers, or configurations.

## 7. Recommended Follow-up Experiments

1.  Collect independent application-side frame timestamps and CPU
    submission/GPU completion measurements.
2.  Capture higher-resolution traces around individual critical FPS
    events.
3.  Repeat gameplay captures with exact surface attach/detach
    timestamps.
4.  Compare individual buffer release events rather than relying only on
    ten-second aggregates.
5.  Repeat tests under the same resolution and graphics configuration to
    improve comparability.
6.  Collect additional sessions to increase the number of critical
    samples.

## 8. Day 7 Completion Checklist

-   [x] Critical versus higher-FPS baseline comparison implemented.
-   [x] Pearson correlation analysis implemented.
-   [x] Diagnostic hypotheses and evidence scoring implemented.
-   [x] Confidence assignment implemented.
-   [x] Actionable investigation recommendations implemented.
-   [x] Recommendation priority counts integrated into runtime output.
-   [x] All modules integrated into `main.cpp`.
-   [x] Nine registered test suites passed.
-   [x] Final Wayland analysis executed successfully.

## 9. Conclusion

Day 7 completed the profiler's diagnostic reasoning pipeline. The
project now moves beyond anomaly detection: it compares performance
groups, measures relationships between telemetry metrics, records
competing hypotheses with supporting and contradictory evidence, and
proposes concrete experiments to investigate them.

The current evidence most strongly motivates further investigation of
presentation lifecycle behavior and independent frame-production
measurements. It does not establish a single root cause for the FPS
drops.

**Day 7 status: COMPLETE.**

------------------------------------------------------------------------

*Generated from the Day 7 implementation and runtime output supplied for
this project. Numerical results reflect the captured dataset and current
analyzer configuration.*
