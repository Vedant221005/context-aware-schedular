# Context-Aware CPU Scheduler (Project Foundation)

This repository contains a Final Year CE research project called "Context-Aware CPU Scheduler". It simulates CPU scheduling algorithms and calculates dynamic process context scores from live system context.

Tech stack:
- C++17
- OOP design
- CMake build

Project layout:

context-aware-scheduler/
- src/         (implementation .cpp files)
- include/     (public headers)
- data/        (input / simulated traces)
- results/     (experiment outputs)
- analysis/    (analysis scripts and notes)
- docs/        (documentation)
- CMakeLists.txt
- README.md

What is included:
- Process class (attributes + display)
- ContextManager class (simulated updates + display)
- ContextScoreEngine class (dynamic context-aware scoring and ranking)
- FCFS, Round Robin, and non-preemptive Priority schedulers
- CSV dataset loading and experimental scheduler comparison
- Metrics class (tracking common scheduling metrics)
- Menu-driven main.cpp demonstration
- A working CMakeLists.txt for building the foundation

Next recommended steps:
1. Connect context scores to a future adaptive scheduling policy.
2. Add power and thermal metric calculations.
3. Expand unit-test coverage for scheduling and context-score behavior.

Experimental evaluation:
- Select option 8 to load `data/workload_50.csv`.
- Results are displayed and exported to `results/comparison_results.csv`.
- Additional datasets are available in `data/workload_100.csv` and
  `data/workload_200.csv`.
