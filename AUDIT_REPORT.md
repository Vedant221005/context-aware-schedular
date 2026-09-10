# Context-Aware CPU Scheduler
# Complete Implementation Audit Report

## 1. Executive Summary

The project is a functional C++17 discrete-event CPU scheduling simulator
containing:

- FCFS scheduling
- Round Robin scheduling
- Non-preemptive Priority scheduling
- Adaptive Context-Aware scheduling
- Aging-based starvation mitigation
- Dynamic context trace loading
- Diagnostics
- Experimental evaluation
- CSV exports

The project currently builds successfully with CMake. However, the existing
test source files are not registered in `CMakeLists.txt`, so CTest reports that
no tests are available.

Overall maturity: **Level 3 of 5 — Functional Research Prototype**.

---

## 2. Implemented Features

## 2.1 FCFS Scheduling

### Files

- `include/FCFS.h`
- `src/FCFS.cpp`
- `include/Process.h`
- `include/SchedulerResult.h`

### Functions and Classes

- `runFCFS`
- `Process`
- `ProcessStat`
- `GanttSegment`
- `SchedulerResult`

### Current Status

FCFS is implemented and operational.

It:

- Sorts processes by arrival time.
- Uses PID as a tie-breaker.
- Executes processes non-preemptively.
- Calculates waiting, turnaround, and response times.
- Calculates throughput and context switches.
- Produces Gantt chart segments.

### Missing Functionality

- No scheduler-specific automated tests.
- No validation for negative burst times or invalid arrival times.
- Metric calculations are duplicated across schedulers.

---

## 2.2 Round Robin Scheduling

### Files

- `include/RoundRobin.h`
- `src/RoundRobin.cpp`
- `include/SchedulerResult.h`

### Functions and Classes

- `runRoundRobin`
- `Process`
- `SchedulerResult`
- `ProcessStat`
- `GanttSegment`

### Current Status

Round Robin is implemented and operational.

It:

- Uses a configurable time quantum.
- Maintains a FIFO ready queue.
- Supports arrivals during execution.
- Produces multiple Gantt segments.
- Tracks context switches.
- Calculates waiting, turnaround, and response times.

### Missing Functionality and Risks

- No validation that the quantum is positive.
- No automated scheduler tests.
- Response-time behavior should be tested for processes arriving during an
  active quantum.
- The implementation initializes `firstResponse` when a process enters the
  ready queue, which can under-report response time for processes that arrive
  during another process's time slice.

---

## 2.3 Priority Scheduling

### Files

- `include/PriorityScheduler.h`
- `src/PriorityScheduler.cpp`
- `include/SchedulerResult.h`

### Functions and Classes

- `runPriorityScheduling`
- `Process`
- `SchedulerResult`
- `ProcessStat`
- `GanttSegment`

### Current Status

Priority scheduling is implemented as non-preemptive priority scheduling.

It:

- Builds the ready queue.
- Selects the lowest numeric priority value.
- Uses arrival time as a tie-breaker.
- Handles idle CPU periods.
- Calculates common scheduling metrics.

### Missing Functionality

- No PID tie-breaker when priority and arrival time are equal.
- Completely equal values depend on sort behavior.
- No automated scheduler tests.
- The meaning of numeric priority should be explicitly documented.

---

## 2.4 Adaptive Context-Aware Scheduling

### Files

- `include/ContextAwareScheduler.h`
- `src/ContextAwareScheduler.cpp`
- `include/ContextScoreEngine.h`
- `src/ContextScoreEngine.cpp`
- `include/ContextManager.h`
- `src/ContextManager.cpp`

### Main Classes and Functions

#### `ContextAwareScheduler`

- `schedule`
- `scheduleInternal`
- `calculateProcessImpact`
- `applyContextSnapshot`
- `displayExecutionOrder`
- `displayContextChanges`
- `exportContextChanges`
- `getDecisionLog`
- `getContextChanges`
- Aging-analysis accessors

#### `ContextScoreEngine`

- `calculateWeights`
- `calculateContextScore`
- `displayContextScoreBreakdown`

#### `ContextManager`

- `updateBatteryLevel`
- `updateTemperature`
- `updateCPUUtilization`
- `updateUserActivity`
- `displayContext`

### Current Status

The adaptive scheduler is implemented and operational.

It:

- Builds a ready queue.
- Calculates context score.
- Calculates process impact.
- Calculates dynamic priority.
- Selects the highest-priority ready process.
- Records decisions and execution order.
- Supports static and dynamic context modes.

The scheduler is non-preemptive.

---

## 2.5 Aging

### Files

- `include/ContextAwareScheduler.h`
- `src/ContextAwareScheduler.cpp`
- `src/ExperimentRunner.cpp`

### Formula

```text
DynamicPriority =
    BasePriority
    + ContextScore / 10
    + ProcessImpact / 10
    + WaitingTime * 0.05
```

### Current Status

Aging is correctly recalculated at each adaptive scheduling decision.

For every ready process:

1. Waiting time is calculated.
2. Waiting time is clamped to zero if necessary.
3. Aging bonus is calculated.
4. Dynamic priority is recomputed.
5. The highest dynamic-priority process is selected.

The scheduler records:

- Maximum waiting time
- Average aging bonus
- Processes rescued by aging

### Missing Functionality

- Aging coefficient `0.05` is hardcoded.
- No configurable aging policy.
- No fairness metric.
- No aging-specific tests.
- No formal starvation-bound analysis.

---

## 2.6 Dynamic Context Trace Support

### Files

- `include/ContextTraceLoader.h`
- `src/ContextTraceLoader.cpp`
- `data/context_trace.csv`
- `include/ContextAwareScheduler.h`
- `src/ContextAwareScheduler.cpp`
- `src/main.cpp`

### Classes and Functions

#### `ContextTraceLoader`

- `loadCSV`

#### `ContextAwareScheduler`

- Trace overload of `schedule`
- `scheduleInternal`
- `applyContextSnapshot`
- `displayContextChanges`
- `exportContextChanges`

### Current Status

Dynamic trace support is implemented and connected to option 7.

It:

- Loads context snapshots from CSV.
- Sorts snapshots by timestamp.
- Updates battery level.
- Updates CPU temperature.
- Updates CPU utilization.
- Updates user activity.
- Recalculates context scores.
- Recalculates dynamic priorities.
- Displays affected processes and priority changes.
- Exports context changes.

### Important Timing Limitation

The scheduler is non-preemptive. Context events are applied:

- Before selecting the next process.
- After a process completes.

If a context timestamp occurs during a long process burst, the update is
applied only after that burst completes. The current process is not
preempted.

This is valid for a non-preemptive simulator, but it should be described as
**scheduling-boundary context updates**, not instantaneous real-time reaction.

### Additional Technical Concern

When a trace event is applied late, `applyContextSnapshot` calculates aging
using the trace timestamp rather than the actual current simulation time. This
can make the recorded priority change temporarily differ from the actual
simulation state.

Dynamic trace support is also not used by the experimental evaluation module.

---

## 2.7 Diagnostics

### Files

- `include/ExperimentRunner.h`
- `src/ExperimentRunner.cpp`
- `src/ContextAwareScheduler.cpp`
- `src/main.cpp`

### Functions

- `ExperimentRunner::runAdaptiveDiagnostics`
- `ContextAwareScheduler::displayExecutionOrder`
- `ContextAwareScheduler::displayContextChanges`
- `ContextAwareScheduler::getDecisionLog`

### Current Status

Diagnostics are implemented and operational.

They display:

- Decision time
- Selected PID
- Waiting time
- Aging bonus
- Dynamic priority
- Maximum waiting time
- Average aging bonus
- Processes rescued by aging
- Context transition values
- Priority changes for affected processes

### Missing Functionality

- Option 9 diagnoses static context only.
- Dynamic trace diagnostics are not integrated into option 9.
- Diagnostics are mainly console-oriented.
- Context-change CSV does not contain affected process IDs or old/new
  priorities.
- No complete machine-readable diagnostic event model exists.

---

## 2.8 Experimental Evaluation

### Files

- `include/ExperimentRunner.h`
- `src/ExperimentRunner.cpp`
- `src/main.cpp`
- `include/DatasetLoader.h`
- `src/DatasetLoader.cpp`

### Functions

- `runExperimentalEvaluation`
- `ExperimentRunner::runExperiment`
- `displayResults`
- `displaySummary`
- `exportResults`

### Current Status

The experimental framework compares:

- FCFS
- Round Robin
- Priority
- Adaptive Context-Aware Scheduler

It reports:

- Average waiting time
- Average turnaround time
- Average response time
- Throughput
- Context switches
- Execution time

### Missing Functionality

- Only one workload is run per invocation.
- Context values are hardcoded.
- Dynamic trace evaluation is not included.
- No repeated trials.
- No confidence intervals.
- No standard deviation.
- No statistical significance testing.
- No aging-coefficient comparison.
- No fairness or starvation metrics.
- No automatic chart generation.

---

## 2.9 CSV Exports

### Files

- `src/ExperimentRunner.cpp`
- `src/ContextAwareScheduler.cpp`
- `results/comparison_results.csv`
- `results/adaptive_decision_log.csv`
- `results/context_change_log.csv`

### Current Status

CSV export is implemented.

#### Comparison Results

Contains scheduler-level performance metrics.

#### Adaptive Decision Log

Contains:

- Time
- Ready queue
- Selected process
- Waiting time
- Aging bonus
- Dynamic priority
- Decision reason

#### Context Change Log

Contains:

- Time
- Battery
- Temperature
- CPU utilization
- User activity

### Missing Functionality

The context log does not export:

- Affected process IDs
- Previous priorities
- New priorities
- Context scores before and after updates
- Process-impact values before and after updates
- Actual application time versus trace timestamp

The comparison CSV contains duplicate metric columns:

- `AvgWaiting` and `AverageWaitingTime`
- `AvgTurnaround` and `AverageTurnaroundTime`
- `AvgResponse` and `AverageResponseTime`

This preserves existing fields but is redundant.

---

## 3. Architecture Review

## 3.1 Text Class Diagram

```text
+----------------------+
| Process              |
+----------------------+
| pid                  |
| arrivalTime          |
| burstTime            |
| priority             |
| cpuUsage             |
| batteryImpact        |
| temperatureImpact    |
| foregroundTask       |
| backgroundTask       |
| contextScore         |
+----------------------+
            |
            v
+----------------------+
| ContextManager       |
+----------------------+
| batteryLevel         |
| cpuTemperature       |
| cpuUtilization       |
| userActive           |
+----------------------+
            |
            v
+----------------------+
| ContextScoreEngine   |
+----------------------+
| calculateWeights     |
| calculateContextScore|
| displayBreakdown     |
+----------------------+
            |
            v
+------------------------------+
| ContextAwareScheduler        |
+------------------------------+
| dynamicProcesses             |
| executionOrder               |
| decisionLog                  |
| contextChanges               |
| schedule                     |
| scheduleInternal             |
| applyContextSnapshot         |
| calculateProcessImpact       |
| displayExecutionOrder        |
| displayContextChanges        |
| exportContextChanges         |
+------------------------------+

+----------------------+
| ContextTraceLoader   |
+----------------------+
| loadCSV              |
+----------------------+
            |
            v
+----------------------+
| ContextSnapshot      |
+----------------------+
| time                 |
| battery              |
| temperature          |
| cpuUtilization       |
| userActive           |
+----------------------+

Traditional schedulers:

+----------------------+
| FCFS                 |
| runFCFS              |
+----------------------+

+----------------------+
| Round Robin          |
| runRoundRobin        |
+----------------------+

+----------------------+
| Priority             |
| runPriorityScheduling|
+----------------------+

+----------------------+
| ExperimentRunner     |
+----------------------+
| runExperiment        |
| runAdaptiveDiagnostics|
| exportResults        |
| displayResults       |
| displaySummary       |
+----------------------+

+----------------------+
| SchedulerResult      |
+----------------------+
| ProcessStat          |
| GanttSegment         |
| aggregate metrics    |
+----------------------+
```

## 3.2 Data Flow

```text
Workload CSV
    |
    v
DatasetLoader
    |
    v
vector<Process>
    |
    +--> FCFS
    +--> Round Robin
    +--> Priority
    |
    +--> ContextAwareScheduler
             |
             +--> ContextManager
             +--> ContextScoreEngine
             +--> Process impact model
             +--> Aging calculation
             +--> Optional ContextTraceLoader
             |
             v
        SchedulerResult
             |
             +--> Console output
             +--> ExperimentRunner
             +--> Decision CSV
             +--> Comparison CSV
             +--> Context change CSV
```

## 3.3 Scheduling Workflow

```text
1. Load workload.
2. Initialize ContextManager.
3. Calculate initial context scores and process impacts.
4. Initialize dynamic priorities.
5. Set simulation time.
6. Apply trace events whose timestamps have been reached.
7. Build the ready queue.
8. Recalculate aging for every ready process.
9. Recalculate dynamic priorities.
10. Select the highest dynamic-priority process.
11. Record process statistics and decision log.
12. Run the process to completion.
13. Apply context events reached during execution.
14. Repeat until all processes finish.
15. Export and display results.
```

---

## 4. Technical Debt

## 4.1 Duplicate Code

Context initialization is repeated in `main.cpp` and `ExperimentRunner.cpp`.

Metric calculations are independently implemented in FCFS, Round Robin,
Priority, and Adaptive scheduling.

CSV parsing logic is duplicated between `DatasetLoader` and
`ContextTraceLoader`.

Console formatting is distributed across multiple files.

## 4.2 Large Functions

`ContextAwareScheduler::scheduleInternal` handles:

- Scheduler initialization
- Context processing
- Trace-event application
- Ready queue construction
- Aging calculation
- Priority selection
- Statistics
- Decision logging
- Process completion

Recommended decomposition:

```text
initializeDynamicProcesses()
applyPendingContextEvents()
buildReadyQueue()
recalculateReadyPriorities()
selectNextProcess()
recordProcessStatistics()
finalizeSchedulerMetrics()
```

`main.cpp` also combines menu handling, workload creation, context setup,
execution, and presentation.

## 4.3 Hardcoded Values

Important hardcoded values include:

- Aging coefficient `0.05`
- Context-score weights
- Battery, temperature, and CPU thresholds
- Round Robin quantum `4`
- Static context values
- Workload paths
- Output paths

These should eventually move into configuration objects or command-line
options.

## 4.4 Missing Abstractions

Recommended abstractions:

- `IScheduler` interface
- Shared metric calculator
- Ready-queue abstraction
- `ContextProvider` interface
- Static and trace context providers
- Scheduling policy comparator
- Configuration object
- Structured scheduling/context event model

## 4.5 Missing Tests

Existing test files:

- `test/test_process.cpp`
- `test/test_metrics.cpp`
- `test/test_contextmanager.cpp`

Existing tests cover constructors and basic field updates only.

Missing tests include:

- FCFS ordering and idle periods
- Round Robin quantum behavior
- Round Robin response time
- Priority selection
- Adaptive priority formula
- Aging calculation
- Aging-based priority inversion
- Starvation rescue count
- Context trace sorting
- Invalid trace rows
- Time-zero context updates
- Updates during long bursts
- Dynamic priority changes
- CSV export schema
- Diagnostics output
- Empty workload behavior
- Duplicate timestamps
- Invalid timestamps
- Invalid context ranges

The tests are not registered in `CMakeLists.txt`, which causes CTest to report:

```text
No tests were found!!!
```

---

## 5. Scheduler Analysis

## 5.1 Preemptive or Non-Preemptive?

The Adaptive Context-Aware Scheduler is **non-preemptive**.

Once a process starts, it runs for its entire burst. Context changes during
the burst do not interrupt it.

Round Robin is the only scheduler that uses time slices.

## 5.2 Is Aging Correctly Applied?

Yes, at dispatch points.

All ready processes receive recalculated waiting time, aging bonus, and
dynamic priority before selection.

The main caveat is delayed trace-event processing: a context event can be
processed after its timestamp, and its logged aging value may use the trace
timestamp rather than the actual current simulation time.

## 5.3 Is Starvation Possible?

For a finite workload, indefinite starvation is unlikely because every
selected process completes and aging increases waiting-process priority.

For an unbounded workload, starvation is reduced but not formally eliminated.
There is no mathematical waiting-time bound or fairness guarantee.

The accurate project claim is **starvation mitigation**, not guaranteed
starvation freedom.

## 5.4 Is Dynamic Trace Support Fully Integrated?

No.

It is integrated into option 7, but:

- Option 9 uses static context.
- Option 8 uses static context.
- `SchedulerResult` does not contain context events.
- Context transitions are not represented as Gantt/event segments.
- Context-change export lacks priority details.
- Updates happen at scheduling boundaries.
- No automated tests validate the feature.

## 5.5 Does the Scheduler React to Context Changes?

Yes, for unfinished processes.

Each applied snapshot causes recalculation of:

- Context score
- Process impact
- Base dynamic priority
- Aging-adjusted dynamic priority

The next dispatch can therefore react to changed battery, temperature, CPU
utilization, and user activity.

The currently executing process is not interrupted.

---

## 6. Research Readiness

## 6.1 What Is Sufficient

The following are sufficient for a strong final-year prototype:

- Multiple baseline schedulers
- Explicit adaptive scheduling formula
- Context-aware scoring
- Process-impact modeling
- Aging-based starvation mitigation
- Dynamic context trace concept
- Comparative metrics
- CSV input and output
- Console diagnostics
- Reproducible workload files
- Project documentation

The project demonstrates a complete research pipeline:

```text
Input workload
-> scheduling policy
-> context-aware decisions
-> metrics
-> diagnostics
-> CSV export
```

## 6.2 Improvements That Would Strengthen the Project

### Automated Testing

Register tests in CMake and add coverage for every scheduler and adaptive
feature.

### Research Methodology

Add:

- Multiple workload sizes
- Multiple runs
- Controlled random seeds
- Standard deviation
- Confidence intervals
- Statistical significance analysis

### Fairness Metrics

Add:

- Maximum waiting time
- Waiting-time variance
- 95th-percentile waiting time
- Jain's fairness index
- Starvation count
- Aging rescue rate

### Static versus Dynamic Context Comparison

Compare static and trace-based execution using:

- Waiting time
- Turnaround time
- Response time
- Throughput
- Context switches
- Fairness
- Context sensitivity

### Parameter Sensitivity

Evaluate aging coefficients such as:

```text
0.01, 0.025, 0.05, 0.10, 0.20
```

### Reproducibility

Record:

- Workload filename
- Context mode
- Aging coefficient
- Context weights
- Round Robin quantum
- Configuration values
- Experiment timestamp

---

## 7. Roadmap

## 7.1 Current Maturity Level

**Level 3 of 5 — Functional Research Prototype**

### Level 1 — Skeleton

Basic classes and menu.

### Level 2 — Functional Simulator

Traditional schedulers and metrics.

### Level 3 — Current State

Adaptive priority, aging, diagnostics, dynamic trace loading, and CSV export.

### Level 4 — Research-Grade Simulator

Requires:

- Registered automated tests
- Configurable parameters
- Repeatable experiments
- Fairness metrics
- Static/dynamic comparisons
- Validated trace semantics

### Level 5 — Production or Real-Time System

Requires:

- Real hardware monitoring
- Real-time context ingestion
- Thread safety
- Runtime scheduling integration
- Performance and safety guarantees

## 7.2 Remaining Work

### High Priority

1. Register existing tests in CMake.
2. Add adaptive scheduler tests.
3. Add context trace loader tests.
4. Clarify delayed context-event semantics.
5. Correct and test Round Robin response-time handling.
6. Add deterministic PID tie-breakers.
7. Add input validation.

### Medium Priority

8. Extract shared metric calculations.
9. Introduce scheduler and context-provider interfaces.
10. Make aging configurable.
11. Add fairness and starvation metrics.
12. Extend context-change CSV with priority deltas.
13. Add dynamic-context diagnostics to option 9.

### Lower Priority

14. Add charts and visualizations.
15. Add command-line configuration.
16. Add real-time context providers.
17. Add automated batch experiments.
18. Add statistical analysis and confidence intervals.

## 7.3 Recommended Next Implementation Step

The recommended next step is:

> Create and register a focused automated test suite for the Adaptive
> Context-Aware Scheduler and Dynamic Context Trace Support.

Priority test cases:

```text
1. Aging bonus equals waitingTime * 0.05.
2. All ready processes are recalculated at every decision.
3. Aging can change the selected process.
4. Context trace rows are loaded and sorted correctly.
5. Context updates change dynamic priorities.
6. Updates during a long burst are applied at the next scheduling boundary.
7. Context-change CSV contains every applied snapshot.
8. Static mode behavior remains unchanged.
9. Experimental evaluation remains unchanged.
10. Round Robin response times are correct for mid-quantum arrivals.
```

This work would convert the current functional prototype into a more
defensible and reproducible research implementation.
