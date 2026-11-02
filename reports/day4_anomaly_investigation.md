# Day 4 --- Wayland/Gamescope Anomaly Investigation

**Project:** DroidDeck Graphics Profiler\
**Scope:** Wayland/Gamescope performance analysis\
**Status:** Investigation and regression validation completed; findings
are observational, not causal proof.

------------------------------------------------------------------------

## 1. Objective

Investigate notable anomalies in the captured Wayland/Gamescope log,
distinguish gameplay samples from surface/window lifecycle transitions,
and ensure the analyzer reports only valid gameplay samples in its
FPS-group comparison.

The investigation focused on:

1.  An isolated **2003.93 ms Render Scene maximum**.
2.  Eastward's **1280×720 → 24×24 surface-size transition** and the
    surrounding FPS collapse.
3.  GPU surface detachment and the difference between an open window and
    an actively presenting surface.

The goal was to preserve the evidence, improve sample classification,
and avoid claiming a root cause that the available log does not
establish.

## 2. Dataset and analysis approach

The raw log is loaded by the Wayland log parser. Stats and performance
records are paired by their `HH:MM:SS` timestamp key because their
millisecond portions can differ. The parser returns chronological
samples and records whether stats and performance data are present.

GPU surface lifecycle messages are parsed separately and applied
chronologically to samples. The analyzer uses the resulting
`surfaceActive` state in addition to the window count when deciding
whether a sample belongs to active gameplay.

The captured dataset contains **88 stats samples**. After applying the
FPS-group eligibility checks, **85 gameplay samples** are included in
the FPS-group comparison.

A sample is eligible for the FPS-group comparison when it has stats and
performance data, at least one open window, an active GPU surface, and a
non-zero GPU game-frame count.

## 3. Investigation A --- 2003.93 ms Render Scene outlier

### Observation

At **18:23:05.643**, the log reports:

  Metric                            Recorded value
  ------------------------------ -----------------
  Frames on screen                             218
  Displayed FPS                               21.8
  GPU frames from games                        408
  Open windows                                   1
  Render Scene average                    13.75 ms
  Render Scene maximum              **2003.93 ms**
  Acquire average / maximum         0.13 / 1.61 ms
  Present average / maximum         1.48 / 9.31 ms
  Fence Wait average / maximum     2.32 / 12.56 ms
  GPU release waits                              0
  Release average / maximum        9.59 / 49.80 ms

The next sample, at **18:23:15.645**, reports approximately **40.1 FPS**
and a Render Scene maximum of **10.41 ms**. The following samples return
to approximately 40 FPS.

The surrounding log includes Steam Big Picture surface-size changes
shortly before the outlier: 32×32 → 1280×800, followed by 1280×800 →
1280×720. A touch event occurs after the outlier.

A search of the captured log found the exact value **2003.93** once.

### Interpretation

This is an isolated and unusually large Render Scene maximum in the
captured data. The surrounding samples return to normal values, so the
available evidence does not show a sustained Render Scene slowdown.

The log does not establish whether this value represents a real stall,
an aggregation/measurement artifact, or another transient condition. The
analyzer therefore preserves the value and reports it rather than
silently filtering it.

**Conclusion:** An isolated Render Scene timing outlier was observed.
Its root cause remains undetermined.

## 4. Investigation B --- Eastward 24×24 surface transition

### Timeline

  -----------------------------------------------------------------------
  Timestamp                           Observation
  ----------------------------------- -----------------------------------
  18:24:52.766                        Window renamed from Steam Big
                                      Picture Mode to Eastward

  18:24:52.773                        Eastward begins presenting GPU
                                      frames through Wayland at 1280×720

  18:24:55.665                        28.7 FPS; 556 GPU frames

  18:25:01.626                        Eastward surface changes from
                                      1280×720 to 24×24

  18:25:04.132                        Pointer lock requested and
                                      acquired; relative pointer created

  18:25:05.669                        1.1 FPS; 4 GPU frames

  18:25:15.670                        4.1 FPS; 70 GPU frames

  18:25:25.671                        16.5 FPS; 318 GPU frames

  18:25:35.672                        39.9 FPS; 795 GPU frames
  -----------------------------------------------------------------------

No explicit transition of the Eastward surface back to 1280×720 was
found during this session. A later 24×24 → 1280×720 transition belongs
to Steam Big Picture Mode.

During the FPS collapse, measured Render Scene timing remains around
4--6 ms. This means the measured compositor Render Scene timing alone
does not explain the reduction in displayed/game GPU frames.

### Interpretation

The 24×24 surface-size transition and pointer-lock activity occur close
in time to a sharp FPS reduction. FPS subsequently recovers in the
recorded samples.

This is a strong temporal correlation, but the log does not establish
that the size transition caused the FPS drop. It also does not explain
why the surface became 24×24 or whether that size was an intentional or
intermediate state.

**Conclusion:** The transition is a useful investigation lead, not a
proven root cause.

## 5. Investigation C --- GPU surface lifecycle and window state

### Timeline

  ----------------------------------------------------------------------------------
  Timestamp                           Observation
  ----------------------------------- ----------------------------------------------
  18:37:25.920                        5.4 FPS; one open window

  18:37:31.749                        Touch event for Eastward

  18:37:35.890                        `gpu screen surface gone: presenting paused`

  18:37:35.921                        One open window; sample reports 29.1 FPS, but
                                      GPU surface is inactive

  18:37:38.923                        Window renamed from Eastward to Steam Big
                                      Picture Mode

  18:37:38.925                        Pointer unlocked; Steam Big Picture surface
                                      changes 24×24 → 1280×720

  18:37:45.354                        Steam Big Picture window closes

  18:37:45.921                        Zero frames on screen, zero FPS, zero open
                                      windows

  18:37:48.468                        Gamescope disconnects
  ----------------------------------------------------------------------------------

### Finding

At **18:37:35.921**, the sample still reports `windowsOpen == 1`, even
though the GPU surface detached at **18:37:35.890**. Therefore, an
open-window count alone is not sufficient to determine whether a sample
represents active GPU presentation.

The later sample at **18:37:45.921** has zero open windows and is
already excluded by the existing window-count filter. The earlier sample
demonstrates why surface lifecycle state must be tracked independently.

### Implementation

The parser now recognizes GPU screen surface attachment and detachment
messages, stores them as lifecycle events, sorts them chronologically,
and applies the most recent event preceding each sample.

The stutter analyzer excludes samples without an active surface. The
FPS-group analysis applies the same active-surface condition.

The parser regression test now checks:

-   Surface active during gameplay at 18:25:15.670.
-   Surface active immediately before detachment at 18:37:25.920.
-   Surface inactive after detachment at 18:37:35.921.
-   The post-detachment sample can still report one open window.

**Conclusion:** Window state and GPU surface presentation state are
distinct signals. The analyzer now tracks both.

## 6. Analyzer results after lifecycle filtering

The latest validated run reports:

  Metric                            Result
  ------------------------------- --------
  Wayland samples loaded                88
  Gameplay samples analyzed             85
  Stutter events detected               67
  FPS degradation events                 7
  Timing anomaly events                 61
  Combined events                        1
  Render Scene timing anomalies          4
  Present timing anomalies               0
  Fence Wait timing anomalies            2
  Release timing anomalies              59

The FPS-group distribution is:

  FPS group                       Samples   Average FPS
  ----------------------------- --------- -------------
  Critical (\<10 FPS)                   5          4.04
  Low (10--\<15 FPS)                    2         13.05
  Intermediate (15--\<30 FPS)          22         24.74
  Higher (≥30 FPS)                     56         36.36
  **Total**                        **85**           ---

The group counts sum to the 85 eligible gameplay samples.

### Release latency by FPS group

The analyzer now reports the actual maximum `releaseMaxMs` observed
within each group, rather than the average of per-sample maximum values.

  FPS group        Average Release latency   Maximum Release latency
  -------------- ------------------------- -------------------------
  Critical                         5.17 ms                  43.46 ms
  Low                             10.88 ms                  55.21 ms
  Intermediate                     8.79 ms                  81.59 ms
  Higher                          14.33 ms                  97.97 ms

These are descriptive group statistics. They do not by themselves prove
that Release latency causes the FPS degradation. In particular, the
current dataset reports zero GPU release waits in each FPS group.

## 7. Code and test changes

The investigation resulted in the following changes:

1.  **Wayland log parser**
    -   Parses GPU screen surface attachment/detachment lifecycle
        messages.
    -   Applies lifecycle state to samples in chronological order.
    -   Keeps lifecycle state separate from window count.
2.  **Stutter analyzer**
    -   Skips samples with no stats, zero open windows, or an inactive
        GPU surface.
3.  **FPS-group analysis**
    -   Uses the same gameplay eligibility conditions, including active
        surface and non-zero GPU frame count.
    -   Reports the number of eligible gameplay samples separately from
        the total number of loaded samples.
    -   Reports the true maximum Release latency per FPS group.
4.  **Regression tests**
    -   Added assertions for active gameplay, pre-detachment and
        post-detachment surface state.
    -   Added an assertion that the window count can remain one after
        surface detachment.
    -   Existing parser and analyzer tests remain in place.

## 8. Validation

The following commands completed successfully:

``` powershell
cmake --build build
ctest --test-dir build --output-on-failure
.\build\frame_analyzer.exe --wayland
```

CTest result:

-   `StatisticsTests` --- Passed
-   `WaylandParserTest` --- Passed
-   `WaylandLogParserTest` --- Passed
-   `StutterAnalyzerTests` --- Passed

**Result: 4/4 test suites passed.**

The latest analyzer output reports 88 loaded samples and 85 gameplay
samples analyzed. The FPS-group counts sum to 85.

## 9. Limitations and follow-up work

The current log supports identifying timing anomalies and temporal
correlations, but it does not establish the root cause of the 24×24
transition, the 2003.93 ms Render Scene outlier, or the Release timing
anomalies.

The analysis is also based on aggregated Wayland/Gamescope samples. A
10-second aggregate can show that an extreme value occurred in an
interval, but it cannot necessarily identify the exact instant or
duration of the underlying event.

Potential follow-up work:

-   Capture a higher-resolution trace around the 24×24 transition.
-   Compare repeated runs to see whether the transition and FPS collapse
    recur together.
-   Inspect the source/semantics of Release timing and held-frame
    counters before assigning subsystem-level causality.
-   Preserve raw measurements and add explicit handling for missing or
    incomplete perf records.
-   Validate the 2003.93 ms outlier against a more granular trace.

## 10. Final conclusion

Day 4 established a more reliable distinction between loaded log records
and valid gameplay samples. It identified an isolated Render Scene
timing outlier, documented a surface-size transition temporally
associated with an FPS collapse, and demonstrated that window count
alone cannot represent active GPU presentation.

The parser and analyzer now account for GPU surface lifecycle state, the
reporting distinguishes 88 loaded samples from 85 gameplay samples, and
Release maximum latency is calculated correctly. All four test suites
pass.

The findings are intentionally framed as observations and correlations.
Further tracing is required before assigning root causes.
