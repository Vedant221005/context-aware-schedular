#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/ContextManager.h"
#include "../include/ContextAwareScheduler.h"
#include "../include/ContextScoreEngine.h"
#include "../include/DatasetLoader.h"
#include "../include/ExperimentRunner.h"
#include "../include/FCFS.h"
#include "../include/Metrics.h"
#include "../include/PriorityScheduler.h"
#include "../include/Process.h"
#include "../include/RoundRobin.h"
#include "../include/SchedulerResult.h"

namespace {

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

void runContextAwareScheduler(const std::vector<Process>& workload) {
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
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }

    return 0;
}
