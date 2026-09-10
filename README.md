# Context-Aware CPU Scheduler

A C++17 discrete-event CPU scheduling simulator that compares traditional
scheduling algorithms with an Adaptive Context-Aware Scheduler.

## Overview

The simulator models CPU workloads and selects processes using both scheduling
metadata and simulated system context. It provides a menu-driven interface for
running individual schedulers, comparing algorithms, inspecting context
scores, diagnosing adaptive decisions, and exporting experimental results.

## Motivation and Objectives

Traditional schedulers generally make decisions using arrival time, burst time,
and static priority. They do not account for the operating environment or the
impact of running a process on that environment. This project explores a more
responsive scheduling policy that:

- Adapts process priority to battery, thermal, utilization, and user-activity
  conditions.
- Models process impact on the system.
- Prevents starvation through priority aging.
- Measures scheduling quality using standard performance metrics.
- Produces reproducible logs and CSV output for analysis.

## Implemented Features

1. FCFS scheduling
2. Round Robin scheduling
3. Priority scheduling
4. Context Score Engine
5. Adaptive Context-Aware Scheduler
6. Dynamic priority calculation
7. Process impact modeling
8. Aging mechanism for starvation prevention
9. CSV workload loader
10. Experimental evaluation framework
11. Scheduler diagnostics
12. Scheduling decision logging
13. CSV result export
14. Comparative performance analysis
15. Dynamic context trace support
16. Adaptive Round Robin with context-based quantum selection
17. Earliest Deadline First scheduling
18. Windows real-time context monitoring
19. Live priority and quantum adaptation

## Context Factors

The current simulated context includes:

- Battery level
- CPU temperature
- CPU utilization
- User activity

The `ContextScoreEngine` combines these factors with process attributes such as
foreground/background status and resource impact to calculate a context score.
The scheduler also calculates a separate process-impact contribution.

## Adaptive Round Robin

The Adaptive Round Robin scheduler extends time-sliced scheduling by selecting
its quantum from the current context instead of using one fixed value. It uses
a base quantum of 6 and clamps the result to the range 2 through 20:

```text
Quantum = BaseQuantum
        + BatteryAdjustment
        + TemperatureAdjustment
        + CPUAdjustment
        + UserActivityAdjustment
```

High battery and active-user conditions reduce the quantum, while low battery,
high temperature, high CPU utilization, and inactive-user conditions increase
it. In dynamic trace mode, the quantum is recalculated whenever a context
snapshot is reached and the new value is used for subsequent time slices.

Option 10 runs Adaptive Round Robin in either static or dynamic context mode.
The scheduler reports quantum changes, ready queues, execution order, and
quantum statistics. Its outputs are exported to
[`results/adaptive_rr_results.csv`](results/adaptive_rr_results.csv) and
[`results/adaptive_rr_quantum_log.csv`](results/adaptive_rr_quantum_log.csv).

## Earliest Deadline First

Option 11 runs the independent Earliest Deadline First scheduler using
[`data/realtime_workload.csv`](data/realtime_workload.csv). EDF selects the
ready process with the earliest deadline, breaking ties by arrival time and
then PID. It reports `MET` or `MISSED` deadline status and exports metrics to
[`results/edf_results.csv`](results/edf_results.csv).

## Adaptive Scheduling Approach

At every scheduling decision, the adaptive scheduler:

1. Builds the ready queue from processes that have arrived.
2. Recalculates the waiting time and aging bonus for every ready process.
3. Recomputes each ready process's dynamic priority.
4. Selects the process with the highest dynamic priority.
5. Records the decision for diagnostics and CSV logging.

The scheduler is non-preemptive: once selected, a process runs until its burst
is complete. FCFS, Round Robin, and Priority scheduling retain their own
existing behavior and are evaluated independently.

### Dynamic Context Trace Mode

Option 7 supports static context and dynamic context trace execution. Dynamic
mode loads [`data/context_trace.csv`](data/context_trace.csv), using:

```text
Time,Battery,Temperature,CPUUtilization,UserActive
0,90,50,45,1
20,80,55,60,1
40,65,70,85,1
60,40,85,90,0
80,25,90,95,0
```

When simulation time reaches a trace timestamp, the scheduler updates the
`ContextManager`, recalculates context scores and dynamic priorities for
unfinished processes, and records the transition. Context transitions display
affected processes and priority changes and are exported to
[`results/context_change_log.csv`](results/context_change_log.csv).

### Dynamic Priority Formula

```text
DynamicPriority =
    BasePriority
    + ContextScore / 10
    + ProcessImpact / 10
    + WaitingTime * 0.05
```

Waiting time is calculated as:

```text
WaitingTime = max(0, CurrentTime - ArrivalTime)
```

The aging bonus gradually raises the priority of processes that remain in the
ready queue, reducing starvation while preserving context awareness.

### Real-Time Context Mode

Option 7 mode 3 (also available as option 12) samples live Windows system
metrics every three seconds. The monitor uses `GetSystemPowerStatus`,
`GetSystemTimes`, and `GetLastInputInfo`. CPU temperature is reported as `-1`
when the platform does not expose a safe portable sensor value; scheduling
continues normally. Context changes trigger priority recalculation for the
Adaptive Context-Aware Scheduler and quantum recalculation for Adaptive Round
Robin. EDF can also be run in real-time mode without changing its deadline
ordering.

Live observations and adaptation events are exported to:

- `results/realtime_context_log.csv`
- `results/realtime_scheduler_log.csv`
- `results/realtime_quantum_log.csv`
- `results/realtime_adaptivity_log.csv`

## Diagnostics and Decision Logging

Option 9, **Diagnose Adaptive Scheduler**, displays each adaptive scheduling
decision with:

- Decision time
- Selected process
- Waiting time
- Aging bonus
- Final dynamic priority

The decision log is exported to
[`results/adaptive_decision_log.csv`](results/adaptive_decision_log.csv).
The diagnostics summary also reports maximum waiting time, average aging bonus,
and the number of processes rescued by aging.

## Experimental Evaluation

Option 8, **Run Experimental Evaluation**, runs the same workload through:

- FCFS
- Round Robin
- Priority
- Adaptive Round Robin
- Adaptive Context-Aware Scheduler with aging
- Earliest Deadline First

The evaluation reports average waiting time, average turnaround time, average
response time, throughput, context switches, and execution time. Results are
exported to
[`results/comparison_results.csv`](results/comparison_results.csv).

### Experimental Results

| Scheduler | Avg Waiting | Avg Turnaround | Avg Response |
| --- | ---: | ---: | ---: |
| FCFS | 208.78 | 219.24 | 208.78 |
| Round Robin | 280.98 | 291.44 | 1.32 |
| Priority | 145.70 | 156.16 | 145.70 |
| Adaptive Round Robin | 286.98 | 297.44 | 62.66 |
| Adaptive Context-Aware | 253.28 | 263.74 | 253.28 |
| EDF | 5.20 | 9.20 | 5.20 |

### Aging Analysis

- **Maximum Waiting Time:** 448
- **Average Aging Bonus:** 12.66
- **Processes Rescued By Aging:** 32

## Project Structure

```text
context-aware-schedular/
├── CMakeLists.txt
├── README.md
├── data/
│   ├── context_trace.csv
│   ├── realtime_workload.csv
│   ├── workload_50.csv
│   ├── workload_100.csv
│   └── workload_200.csv
├── docs/
├── include/
├── results/
│   ├── adaptive_decision_log.csv
│   ├── comparison_results.csv
│   └── edf_results.csv
├── src/
│   ├── ContextAwareScheduler.cpp
│   ├── ContextManager.cpp
│   ├── ContextScoreEngine.cpp
│   ├── ContextTraceLoader.cpp
│   ├── RealTimeContextMonitor.cpp
│   ├── EDFScheduler.cpp
│   ├── DatasetLoader.cpp
│   └── ExperimentRunner.cpp
└── test/
```

### Important Source Files

- `src/ContextAwareScheduler.cpp` — adaptive scheduling and aging logic
- `src/ContextManager.cpp` — simulated system context
- `src/ContextScoreEngine.cpp` — context score calculation
- `src/ContextTraceLoader.cpp` — dynamic context trace loading
- `src/AdaptiveRoundRobin.cpp` — context-adaptive time-slice scheduling
- `src/DatasetLoader.cpp` — CSV workload loading
- `src/ExperimentRunner.cpp` — evaluation, diagnostics, and exports
- `src/RealTimeContextMonitor.cpp` — Windows live context collection and safe fallbacks

## Requirements

- C++17-compatible compiler
- CMake 3.10 or newer
- Standard build tools for the selected compiler

## Build Instructions

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

On Windows with a Visual Studio generator, use:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Execution

Run the generated executable from the project root so relative paths to
`data/` and `results/` resolve correctly:

```bash
./build/context-aware-scheduler
```

On Windows:

```powershell
.\build\context-aware-scheduler.exe
```

### Sample Menu Output

```text
Context-Aware CPU Scheduler
1. Run FCFS
2. Run Round Robin
3. Run Priority Scheduling
4. Compare All Algorithms
5. Exit
6. Calculate Context Scores
7. Run Context-Aware Scheduler
8. Run Experimental Evaluation
9. Diagnose Adaptive Scheduler
10. Run Adaptive Round Robin
11. Run EDF Scheduler
Select an option:
```

After selecting option 7:

```text
Context Mode
1. Static Context Mode
2. Dynamic Context Trace Mode
Select a mode:
```

Use option 7 to view adaptive execution and aging analysis, option 8 to run
the comparative evaluation, option 9 to inspect the detailed adaptive
decision log, and option 10 to run Adaptive Round Robin.
Option 11 runs EDF and reports deadline misses.

## Screenshots

Add screenshots of the running simulator here:

```text
![Main menu](docs/screenshots/main-menu.png)
![Adaptive scheduler output](docs/screenshots/adaptive-scheduler.png)
![Experimental evaluation](docs/screenshots/experimental-evaluation.png)
```

## Future Work

- Real-time battery monitoring
- Real-time CPU utilization
- Real-time CPU temperature
- Live user activity detection
- Dynamic context updates during execution
- Larger workload and benchmark suites
- Expanded automated scheduler tests
- Visualization of Gantt charts and decision metrics

## Contributing

Contributions are welcome. To propose a change:

1. Fork the repository.
2. Create a focused feature branch.
3. Implement and test the change.
4. Keep scheduler behavior changes isolated and documented.
5. Open a pull request with a clear summary and validation results.

Please preserve the existing C++17 style and avoid changing traditional
scheduler behavior when working on adaptive-scheduler features.

## License

This project does not currently declare a software license. Until a license is
added to the repository, all rights are reserved by the project author.
