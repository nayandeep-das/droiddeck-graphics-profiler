# Daily Progress Report — Day 3

**Project:** DroidDeck Graphics Profiler  
**Date:** November 1, 2026  
**Focus:** FPS grouping, subsystem comparison and anomaly correlation.

## 1. Objective
Investigate whether low displayed FPS correlates with increased compositor subsystem latency or reduced game GPU-frame production.

## 2. Work completed

- Implemented four FPS performance categories.
- Added subsystem timing accumulation and comparison.
- Integrated the analysis into Wayland mode.
- Filtered samples without valid stats, performance data or game GPU frames.
- Investigated critical FPS intervals.
- Inspected raw Wayland logs around severe performance drops.
- Identified surface-size transitions and window lifecycle events.

## 3. FPS group comparison

| Category | Samples | Average FPS |
|---|---:|---:|
| Critical (<10 FPS) | 6 | 3.37 |
| Low (10–15 FPS) | 2 | 13.05 |
| Intermediate (15–30 FPS) | 23 | 24.93 |
| Higher (>=30 FPS) | 56 | 36.36 |

## 4. Subsystem timing comparison

| Subsystem | Critical FPS | Higher FPS |
|---|---:|---:|
| Render Scene | 4.72 ms | 6.66 ms |
| Acquire | 0.12 ms | 0.19 ms |
| Present | 1.07 ms | 1.35 ms |
| Fence Wait | 2.86 ms | 4.31 ms |
| Release | 4.74 ms | 14.33 ms |

The critical-FPS group did not show consistently higher average subsystem latency.

## 5. Important observations

1. Game GPU-frame production generally moved in the same direction as displayed FPS.
2. At 18:25:01, Eastward changed its frame size from 1280x720 to 24x24.
3. At 18:25:05, displayed FPS dropped to 1.1.
4. At 18:25:35, displayed FPS recovered to 39.9.
5. Steam Big Picture initialization showed different frame-size transitions.
6. A Render Scene maximum of 2003.93 ms was observed at 18:23:05.
7. Steam Big Picture closed at 18:37:45, representing a window lifecycle transition.

## 6. Current interpretation

The available measurements do not establish a single compositor subsystem as the cause of FPS degradation.

The 24x24 surface-size transition is an investigation lead, not a confirmed cause.

The relationship between game-frame production, surface-size changes and displayed FPS requires further validation.

## 7. Pending work

- Investigate the 24x24 surface transition.
- Investigate the 2003.93 ms Render Scene outlier.
- Separate window lifecycle events from active gameplay measurements.
- Validate the observations with additional benchmark captures.

## 8. Outcome

Completed the initial FPS and subsystem correlation analysis and identified specific anomalies for further investigation.

**Status:** Analysis complete; written report and further validation pending.