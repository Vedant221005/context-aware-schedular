#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "../include/ContextManager.h"
#include "../include/ContextAwareScheduler.h"
#include "../include/ContextScoreEngine.h"
#include "../include/ContextTraceLoader.h"
#include "../include/AdaptiveRoundRobin.h"
#include "../include/EDFScheduler.h"
#include "../include/DatasetLoader.h"
#include "../include/ExperimentRunner.h"
#include "../include/FCFS.h"
#include "../include/Metrics.h"
#include "../include/PriorityScheduler.h"
#include "../include/Process.h"
#include "../include/RoundRobin.h"
#include "../include/RealTimeContextMonitor.h"
#include "../include/SchedulerResult.h"

namespace {

void exportRealtimeLogs(
    const RealTimeContextMonitor& monitor,
    const std::vector<ContextChange>& changes,
    const std::vector<AdaptiveQuantumEvent>& quantumEvents);

std::vector<Process> buildWorkload() {
    std::vector<Process> workload = {
        Process(1, 0, 10, 8),
        Process(2, 2, 5, 3),
        Process(3, 4, 12, 7),
        Process(4, 5, 8, 5),
        Process(5, 7, 15, 9)
    };

    workload[0].foregroundTask = true;
    workload[0].backgroundTask = false;
    workload[0].cpuUsage = 90.0;
    workload[0].batteryImpact = 90.0;
    workload[0].temperatureImpact = 90.0;
    workload[1].backgroundTask = true;
    workload[1].cpuUsage = 10.0;
    workload[1].batteryImpact = 10.0;
    workload[1].temperatureImpact = 10.0;
    workload[2].backgroundTask = true;
    workload[2].cpuUsage = 5.0;
    workload[2].batteryImpact = 5.0;
    workload[2].temperatureImpact = 5.0;
    workload[3].backgroundTask = true;
    workload[3].cpuUsage = 10.0;
    workload[3].batteryImpact = 10.0;
    workload[3].temperatureImpact = 10.0;
    workload[4].backgroundTask = true;
    workload[4].cpuUsage = 80.0;
    workload[4].batteryImpact = 80.0;
    workload[4].temperatureImpact = 80.0;
    return workload;
}

void printGanttChart(const SchedulerResult& result) {
    std::cout << "\nGantt Chart:\n";
    if (result.gantt.empty()) {
        std::cout << "  No execution segments available.\n";
        return;
    }

    for (const auto& segment : result.gantt) {
        std::cout << "[" << segment.startTime << "-" << segment.endTime << "] P"
                  << segment.pid << " ";
    }
    std::cout << "\n";
}

void printPerProcessStats(const SchedulerResult& result) {
    std::cout << "\nPer-process statistics:\n";
    std::cout << "PID  Arrival  Burst  Priority  Start  Finish  Wait  Turnaround  Response\n";
    std::cout << "---------------------------------------------------------------------\n";

    for (const auto& stat : result.processStats) {
        std::cout << std::left << std::setw(4) << stat.pid
                  << std::setw(8) << stat.arrivalTime
                  << std::setw(6) << stat.burstTime
                  << std::setw(9) << stat.priority
                  << std::setw(6) << stat.startTime
                  << std::setw(7) << stat.completionTime
                  << std::setw(5) << stat.waitingTime
                  << std::setw(11) << stat.turnaroundTime
                  << std::setw(9) << stat.responseTime << "\n";
    }
}

void printOverallStats(const SchedulerResult& result) {
    std::cout << "\nOverall statistics:\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Average Waiting Time      : " << result.avgWaitingTime << "\n";
    std::cout << "Average Turnaround Time   : " << result.avgTurnaroundTime << "\n";
    std::cout << "Average Response Time     : " << result.avgResponseTime << "\n";
    std::cout << "Throughput               : " << result.throughput << " processes/unit time\n";
    std::cout << "Context Switch Count     : " << result.contextSwitches << "\n";
    std::cout << "Total Execution Time     : " << result.totalExecutionTime << "\n";
}

void runAndDisplay(const SchedulerResult& result) {
    std::cout << "\n========================================\n";
    std::cout << result.schedulerName << "\n";
    std::cout << "========================================\n";
    printGanttChart(result);
    printPerProcessStats(result);
    printOverallStats(result);
}

void compareAllAlgorithms(const std::vector<Process>& workload) {
    const auto fcfsResult = runFCFS(workload);
    const auto rrResult = runRoundRobin(workload, 4);
    const auto priorityResult = runPriorityScheduling(workload);
    ContextManager context;
    context.updateBatteryLevel(90.0);
    context.updateTemperature(50.0);
    context.updateCPUUtilization(45.0);
    context.updateUserActivity(true);
    ContextAwareScheduler contextAwareScheduler;
    const auto adaptiveResult = contextAwareScheduler.schedule(workload, context);

    runAndDisplay(fcfsResult);
    runAndDisplay(rrResult);
    runAndDisplay(priorityResult);
    contextAwareScheduler.displayExecutionOrder();
    runAndDisplay(adaptiveResult);

    std::cout << "\nComparison:\n";
    std::cout << "Algorithm                Avg WT     Avg TAT     Avg RT     Throughput  Switches\n";
    std::cout << "-------------------------------------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::left << std::setw(25) << "FCFS"
              << std::setw(11) << fcfsResult.avgWaitingTime
              << std::setw(12) << fcfsResult.avgTurnaroundTime
              << std::setw(11) << fcfsResult.avgResponseTime
              << std::setw(12) << fcfsResult.throughput
              << fcfsResult.contextSwitches << "\n";
    std::cout << std::left << std::setw(25) << "Round Robin"
              << std::setw(11) << rrResult.avgWaitingTime
              << std::setw(12) << rrResult.avgTurnaroundTime
              << std::setw(11) << rrResult.avgResponseTime
              << std::setw(12) << rrResult.throughput
              << rrResult.contextSwitches << "\n";
    std::cout << std::left << std::setw(25) << "Priority"
              << std::setw(11) << priorityResult.avgWaitingTime
              << std::setw(12) << priorityResult.avgTurnaroundTime
              << std::setw(11) << priorityResult.avgResponseTime
              << std::setw(12) << priorityResult.throughput
              << priorityResult.contextSwitches << "\n";
    std::cout << std::left << std::setw(25) << "Adaptive Context-Aware"
              << std::setw(11) << adaptiveResult.avgWaitingTime
              << std::setw(12) << adaptiveResult.avgTurnaroundTime
              << std::setw(11) << adaptiveResult.avgResponseTime
              << std::setw(12) << adaptiveResult.throughput
              << adaptiveResult.contextSwitches << "\n";
}

void displayContextScenario(
    const std::string& name,
    double batteryLevel,
    double temperature,
    double cpuUtilization,
    bool userActive,
    const std::vector<Process>& workload,
    ContextScoreEngine& scoreEngine) {
    ContextManager context;
    context.updateBatteryLevel(batteryLevel);
    context.updateTemperature(temperature);
    context.updateCPUUtilization(cpuUtilization);
    context.updateUserActivity(userActive);

    std::cout << "\n==========================\n";
    std::cout << name << "\n";
    std::cout << "==========================\n";
    std::cout << "Battery Level   : " << batteryLevel << "\n";
    std::cout << "Temperature     : " << temperature << "\n";
    std::cout << "CPU Utilization : " << cpuUtilization << "\n";
    std::cout << "User Active     : " << (userActive ? "YES" : "NO") << "\n";

    std::vector<ProcessScore> scores;
    for (const auto& process : workload) {
        scoreEngine.displayContextScoreBreakdown(process, context);
        scores.push_back({process.pid, scoreEngine.calculateContextScore(process, context)});
    }

    std::sort(scores.begin(), scores.end(), [](const ProcessScore& left, const ProcessScore& right) {
        if (left.score == right.score) {
            return left.pid < right.pid;
        }
        return left.score > right.score;
    });

    std::cout << "\n" << name << " Ranking\n";
    std::cout << "------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t rank = 0; rank < scores.size(); ++rank) {
        std::cout << rank + 1 << ". PID " << scores[rank].pid
                  << " -> " << scores[rank].score << "\n";
    }
}

void calculateContextScores(const std::vector<Process>& workload) {
    ContextScoreEngine scoreEngine;
    displayContextScenario("SCENARIO 1", 90.0, 50.0, 45.0, true, workload, scoreEngine);
    displayContextScenario("SCENARIO 2", 15.0, 85.0, 90.0, false, workload, scoreEngine);
}

void runAdaptivityValidation(
    const std::vector<Process>& workload,
    const ContextManager& initialContext,
    const SchedulerResult& dynamicResult,
    const ContextAwareScheduler& dynamicScheduler) {
    const auto& changes = dynamicScheduler.getContextChanges();
    if (changes.size() < 2) {
        std::cerr << "Adaptivity validation requires an initial and changed "
                     "context snapshot.\n";
        return;
    }

    const ContextChange& change = changes[1];
    std::vector<int> beforeOrder;
    std::vector<int> afterOrder;
    for (const auto& segment : dynamicResult.gantt) {
        if (segment.startTime < change.appliedAt) {
            beforeOrder.push_back(segment.pid);
        } else {
            afterOrder.push_back(segment.pid);
        }
    }

    ContextAwareScheduler staticScheduler;
    const SchedulerResult staticResult =
        staticScheduler.schedule(workload, initialContext);
    const std::size_t comparisonIndex = beforeOrder.size();
    const bool schedulingOrderChanged =
        dynamicResult.gantt.size() != staticResult.gantt.size()
        || std::equal(
            dynamicResult.gantt.begin(),
            dynamicResult.gantt.end(),
            staticResult.gantt.begin(),
            [](const GanttSegment& dynamicSegment,
               const GanttSegment& staticSegment) {
                return dynamicSegment.pid == staticSegment.pid;
            }) == false;
    const bool readyQueueReordered =
        change.readyQueueBefore != change.readyQueueAfter;
    const bool runningProcessReplaced = false;

    std::cout << "\n====================================================\n";
    std::cout << "ADAPTIVITY VALIDATION\n";
    std::cout << "====================================================\n";
    std::cout << "Current simulation time: " << change.appliedAt << "\n";
    std::cout << "Current running process: P" << change.runningPid << "\n";
    std::cout << "\nReady Queue Before Context Update: ";
    for (const int pid : change.readyQueueBefore) {
        std::cout << "P" << pid << " ";
    }
    std::cout << "\nReady Queue After Context Update: ";
    for (const int pid : change.readyQueueAfter) {
        std::cout << "P" << pid << " ";
    }
    std::cout << "\n\nDynamic Priorities Before Update:\n";
    for (const auto& priority : change.prioritiesBefore) {
        std::cout << "P" << priority.pid << "=" << priority.priority << " ";
    }
    std::cout << "\nDynamic Priorities After Update:\n";
    for (const auto& priority : change.prioritiesAfter) {
        std::cout << "P" << priority.pid << "=" << priority.priority << " ";
    }
    std::cout << "\n\nExecution Order Before Context Change:\n";
    for (const int pid : beforeOrder) {
        std::cout << "P" << pid << " ";
    }
    std::cout << "\nExecution Order After Context Change:\n";
    for (const int pid : afterOrder) {
        std::cout << "P" << pid << " ";
    }
    std::cout << "\n\nWas Scheduling Order Changed? "
              << (schedulingOrderChanged ? "YES" : "NO") << "\n";
    std::cout << "Was Ready Queue Reordered? "
              << (readyQueueReordered ? "YES" : "NO") << "\n";
    std::cout << "Was Running Process Replaced? "
              << (runningProcessReplaced ? "YES" : "NO") << "\n";

    if (readyQueueReordered && schedulingOrderChanged
        && comparisonIndex < staticResult.gantt.size()
        && comparisonIndex < dynamicResult.gantt.size()
        && dynamicResult.gantt[comparisonIndex].pid
            != staticResult.gantt[comparisonIndex].pid) {
        std::cout << "\nSUCCESS:\nRuntime re-scheduling detected.\n"
                     "Scheduler adapts execution decisions based on changing "
                     "system context.\n";
    } else {
        std::cout << "\nWARNING:\nContext changes affected priorities but did "
                     "not affect scheduling decisions.\n"
                     "Scheduler is priority-adaptive but not runtime-adaptive.\n";
    }

    std::filesystem::create_directories("results");
    std::ofstream report("results/adaptivity_validation_report.csv");
    if (!report.is_open()) {
        std::cerr << "Unable to write adaptivity validation report.\n";
        return;
    }
    report << "EventTime,AppliedAt,RunningProcess,PID,PriorityBefore,"
               "PriorityAfter,ReadyQueueBefore,ReadyQueueAfter,"
               "WasSchedulingOrderChanged,WasReadyQueueReordered,"
               "WasRunningProcessReplaced\n";
    auto queueText = [](const std::vector<int>& queue) {
        std::ostringstream output;
        for (std::size_t i = 0; i < queue.size(); ++i) {
            if (i > 0) {
                output << " ";
            }
            output << "P" << queue[i];
        }
        return output.str();
    };
    for (const auto& before : change.prioritiesBefore) {
        double afterValue = 0.0;
        for (const auto& after : change.prioritiesAfter) {
            if (after.pid == before.pid) {
                afterValue = after.priority;
                break;
            }
        }
        report << change.snapshot.time << "," << change.appliedAt << ",P"
               << change.runningPid << ",P" << before.pid << ","
               << before.priority << "," << afterValue << ",\""
               << queueText(change.readyQueueBefore) << "\",\""
               << queueText(change.readyQueueAfter) << "\","
               << (schedulingOrderChanged ? "YES" : "NO") << ","
               << (readyQueueReordered ? "YES" : "NO") << ","
               << (runningProcessReplaced ? "YES" : "NO") << "\n";
    }
    std::cout << "\nAdaptivity validation exported to "
                 "results/adaptivity_validation_report.csv\n";
}

void runContextAwareScheduler(const std::vector<Process>& workload) {
    std::cout << "\nContext Mode\n";
    std::cout << "1. Static Context Mode\n";
    std::cout << "2. Dynamic Context Trace Mode\n";
    std::cout << "3. Real-Time Context Mode\n";
    std::cout << "Select a mode: ";
    int mode = 0;
    std::cin >> mode;

    if (mode == 3) {
        RealTimeContextMonitor monitor;
        ContextManager initialContext;
        initialContext.updateBatteryLevel(100.0);
        initialContext.updateTemperature(-1.0);
        initialContext.updateCPUUtilization(0.0);
        initialContext.updateUserActivity(true);
        ContextAwareScheduler scheduler;
        std::cout << "\nStarting real-time context monitoring (3 second refresh).\n";
        const SchedulerResult result =
            scheduler.scheduleRealtime(workload, initialContext, monitor);
        scheduler.displayExecutionOrder();
        printOverallStats(result);
        scheduler.displayContextChanges();
        exportRealtimeLogs(monitor, scheduler.getContextChanges(), {});
        return;
    }

    if (mode == 2) {
        ContextTraceLoader traceLoader;
        const std::vector<ContextSnapshot> trace =
            traceLoader.loadCSV("data/context_trace.csv");
        if (trace.empty()) {
            std::cerr << "Dynamic context mode stopped: context_trace.csv is "
                         "empty or unavailable.\n";
            return;
        }

        ContextManager initialContext;
        initialContext.updateBatteryLevel(90.0);
        initialContext.updateTemperature(50.0);
        initialContext.updateCPUUtilization(45.0);
        initialContext.updateUserActivity(true);

        ContextAwareScheduler scheduler;
        const SchedulerResult result =
            scheduler.schedule(workload, initialContext, trace);
        scheduler.displayExecutionOrder();
        scheduler.displayContextChanges();
        printOverallStats(result);
        scheduler.exportContextChanges("results/context_change_log.csv");
        runAdaptivityValidation(workload, initialContext, result, scheduler);
        std::cout << "\nContext changes exported to "
                     "results/context_change_log.csv\n";
        return;
    }

    if (mode != 1) {
        std::cout << "Invalid context mode.\n";
        return;
    }

    ContextAwareScheduler scheduler;

    ContextManager scenarioOne;
    scenarioOne.updateBatteryLevel(90.0);
    scenarioOne.updateTemperature(50.0);
    scenarioOne.updateCPUUtilization(45.0);
    scenarioOne.updateUserActivity(true);
    std::cout << "\n==========================\nSCENARIO 1\n==========================\n";
    const auto scenarioOneResult = scheduler.schedule(workload, scenarioOne);
    scheduler.displayExecutionOrder();
    printOverallStats(scenarioOneResult);

    ContextManager scenarioTwo;
    scenarioTwo.updateBatteryLevel(15.0);
    scenarioTwo.updateTemperature(85.0);
    scenarioTwo.updateCPUUtilization(90.0);
    scenarioTwo.updateUserActivity(false);
    std::cout << "\n==========================\nSCENARIO 2\n==========================\n";
    const auto scenarioTwoResult = scheduler.schedule(workload, scenarioTwo);
    scheduler.displayExecutionOrder();
    printOverallStats(scenarioTwoResult);

    auto printOrder = [](const SchedulerResult& result) {
        for (std::size_t i = 0; i < result.gantt.size(); ++i) {
            if (i > 0) {
                std::cout << " -> ";
            }
            std::cout << "P" << result.gantt[i].pid;
        }
        std::cout << "\n";
    };

    std::cout << "\n============================\n";
    std::cout << "ORDER COMPARISON\n";
    std::cout << "============================\n";
    std::cout << "Scenario 1:\n";
    printOrder(scenarioOneResult);
    std::cout << "Scenario 2:\n";
    printOrder(scenarioTwoResult);
    std::cout << "\nProcesses moved due to battery, thermal, CPU utilization, "
                 "and user activity changes.\n";
}

void runExperimentalEvaluation() {
    DatasetLoader loader;
    const std::vector<Process> dataset = loader.loadCSV("data/workload_50.csv");
    if (dataset.empty()) {
        std::cerr << "Experimental evaluation stopped: workload_50.csv is empty or unavailable.\n";
        return;
    }

    ExperimentRunner runner;
    runner.runExperiment(dataset);
}

void runAdaptiveRoundRobin(const std::vector<Process>& workload) {
    std::cout << "\nAdaptive Round Robin Mode\n";
    std::cout << "1. Static Context Mode\n";
    std::cout << "2. Dynamic Context Trace Mode\n";
    std::cout << "3. Real-Time Context Mode\n";
    std::cout << "Select a mode: ";
    int mode = 0;
    std::cin >> mode;

    ContextManager context;
    context.updateBatteryLevel(90.0);
    context.updateTemperature(50.0);
    context.updateCPUUtilization(45.0);
    context.updateUserActivity(true);

    AdaptiveRoundRobin scheduler;
    SchedulerResult result;
    if (mode == 3) {
        RealTimeContextMonitor monitor;
        std::cout << "\nStarting real-time context monitoring (3 second refresh).\n";
        result = scheduler.scheduleRealtime(workload, context, monitor);
        scheduler.displayDiagnostics();
        runAndDisplay(result);
        scheduler.exportResults("results/adaptive_rr_results.csv", result);
        scheduler.exportQuantumLog("results/adaptive_rr_quantum_log.csv");
        exportRealtimeLogs(monitor, {}, scheduler.getQuantumLog());
        std::cout << "\nAdaptive Round Robin CSV exports completed.\n";
        return;
    } else if (mode == 2) {
        ContextTraceLoader loader;
        const std::vector<ContextSnapshot> trace =
            loader.loadCSV("data/context_trace.csv");
        if (trace.empty()) {
            std::cerr << "Adaptive Round Robin stopped: context trace is "
                         "empty or unavailable.\n";
            return;
        }

        result = scheduler.schedule(workload, context, trace);
    } else if (mode == 1) {
        result = scheduler.schedule(workload, context);
    } else {
        std::cout << "Invalid Adaptive Round Robin mode.\n";
        return;
    }

    scheduler.displayDiagnostics();
    runAndDisplay(result);
    scheduler.exportResults("results/adaptive_rr_results.csv", result);
    scheduler.exportQuantumLog("results/adaptive_rr_quantum_log.csv");
    std::cout << "\nAdaptive Round Robin CSV exports completed.\n";
}

void exportRealtimeLogs(
    const RealTimeContextMonitor& monitor,
    const std::vector<ContextChange>& changes,
    const std::vector<AdaptiveQuantumEvent>& quantumEvents) {
    std::filesystem::create_directories("results");
    std::ofstream contextLog("results/realtime_context_log.csv");
    contextLog << "Timestamp,Battery,Temperature,CPUUtilization,UserActive\n";
    for (const Context& context : monitor.getSamples()) {
        contextLog << context.timestamp << "," << context.batteryLevel << ","
                   << context.cpuTemperature << "," << context.cpuUtilization
                   << "," << (context.userActive ? "YES" : "NO") << "\n";
    }

    std::ofstream schedulerLog("results/realtime_scheduler_log.csv");
    schedulerLog << "Time,Event,RunningProcess,PriorityChanges\n";
    const auto& samples = monitor.getSamples();
    for (std::size_t i = 1; i < samples.size(); ++i) {
        const Context& previous = samples[i - 1];
        const Context& current = samples[i];
        if ((previous.batteryLevel >= 20.0) != (current.batteryLevel >= 20.0)) {
            schedulerLog << current.timestamp << ",BATTERY_THRESHOLD,-1,0\n";
        }
        if ((previous.cpuUtilization <= 80.0) != (current.cpuUtilization <= 80.0)) {
            schedulerLog << current.timestamp << ",CPU_OVERLOAD,-1,0\n";
        }
        if ((previous.cpuTemperature <= 80.0)
            != (current.cpuTemperature <= 80.0)) {
            schedulerLog << current.timestamp << ",THERMAL_STRESS,-1,0\n";
        }
        if (previous.userActive != current.userActive) {
            schedulerLog << current.timestamp << ",USER_ACTIVITY_TRANSITION,-1,0\n";
        }
    }
    for (const ContextChange& change : changes) {
        schedulerLog << change.appliedAt << ",CONTEXT_CHANGE,"
                     << change.runningPid << "," << change.priorityChanges.size()
                     << "\n";
    }

    std::ofstream quantumLog("results/realtime_quantum_log.csv");
    quantumLog << "Time,Battery,Temperature,CPUUtilization,UserActive,Quantum\n";
    for (const AdaptiveQuantumEvent& event : quantumEvents) {
        quantumLog << event.time << "," << event.context.battery << ","
                   << event.context.temperature << ","
                   << event.context.cpuUtilization << ","
                   << (event.context.userActive ? "YES" : "NO") << ","
                   << event.quantum << "\n";
    }

    std::ofstream adaptivityLog("results/realtime_adaptivity_log.csv");
    adaptivityLog << "Time,Event,Details\n";
    for (const ContextChange& change : changes) {
        adaptivityLog << change.appliedAt << ",PRIORITY_CHANGE,"
                      << change.priorityChanges.size() << " processes\n";
    }
    for (const AdaptiveQuantumEvent& event : quantumEvents) {
        adaptivityLog << event.time << ",QUANTUM_UPDATE,"
                      << event.quantum << "\n";
    }
}

void runEDFScheduler() {
    EDFScheduler scheduler;
    const std::vector<Process> workload =
        scheduler.loadRealtimeWorkload("data/realtime_workload.csv");
    if (workload.empty()) {
        std::cerr << "EDF stopped: realtime_workload.csv is empty or unavailable.\n";
        return;
    }
    std::cout << "\nEDF Mode\n1. Static Mode\n2. Real-Time Context Mode\nSelect a mode: ";
    int mode = 0;
    std::cin >> mode;
    RealTimeContextMonitor monitor;
    const SchedulerResult result = mode == 2
        ? scheduler.scheduleRealtime(workload, monitor)
        : scheduler.schedule(workload);
    scheduler.display(result);
    scheduler.exportResults("results/edf_results.csv", result);
    if (mode == 2) {
        exportRealtimeLogs(monitor, {}, {});
    }
    std::cout << "\nEDF results exported to results/edf_results.csv\n";
}

}  // namespace

int main() {
    const std::vector<Process> workload = buildWorkload();

    while (true) {
        std::cout << "\nContext-Aware CPU Scheduler\n";
        std::cout << "1. Run FCFS\n";
        std::cout << "2. Run Round Robin\n";
        std::cout << "3. Run Priority Scheduling\n";
        std::cout << "4. Compare All Algorithms\n";
        std::cout << "5. Exit\n";
        std::cout << "6. Calculate Context Scores\n";
        std::cout << "7. Run Context-Aware Scheduler\n";
        std::cout << "8. Run Experimental Evaluation\n";
        std::cout << "9. Diagnose Adaptive Scheduler\n";
        std::cout << "10. Run Adaptive Round Robin\n";
        std::cout << "11. Run EDF Scheduler\n";
        std::cout << "12. Run Real-Time Context Mode\n";
        std::cout << "Select an option: ";

        int choice = 0;
        std::cin >> choice;

        switch (choice) {
            case 1:
                runAndDisplay(runFCFS(workload));
                break;
            case 2:
                runAndDisplay(runRoundRobin(workload, 4));
                break;
            case 3:
                runAndDisplay(runPriorityScheduling(workload));
                break;
            case 4:
                compareAllAlgorithms(workload);
                break;
            case 5:
                std::cout << "Exiting scheduler demo.\n";
                return 0;
            case 6:
                calculateContextScores(workload);
                break;
            case 7:
                runContextAwareScheduler(workload);
                break;
            case 8:
                runExperimentalEvaluation();
                break;
            case 9: {
                DatasetLoader loader;
                const std::vector<Process> dataset =
                    loader.loadCSV("data/workload_50.csv");
                ExperimentRunner runner;
                runner.runAdaptiveDiagnostics(dataset);
                break;
            }
            case 10:
                runAdaptiveRoundRobin(workload);
                break;
            case 11:
                runEDFScheduler();
                break;
            case 12:
                runContextAwareScheduler(workload);
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }

    return 0;
}
