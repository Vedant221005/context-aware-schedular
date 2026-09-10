#include "../include/ExperimentRunner.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../include/ContextAwareScheduler.h"
#include "../include/ContextManager.h"
#include "../include/AdaptiveRoundRobin.h"
#include "../include/EDFScheduler.h"
#include "../include/FCFS.h"
#include "../include/PriorityScheduler.h"
#include "../include/RoundRobin.h"

namespace {

ExperimentResult fromSchedulerResult(
    const std::string& name,
    const SchedulerResult& result) {
    return {
        name,
        result.avgWaitingTime,
        result.avgTurnaroundTime,
        result.avgResponseTime,
        result.throughput,
        result.contextSwitches,
        static_cast<double>(result.totalExecutionTime)
    };
}

}  // namespace

void ExperimentRunner::runExperiment(const std::vector<Process>& workload) {
    if (workload.empty()) {
        std::cerr << "Cannot run an experiment with an empty workload.\n";
        return;
    }

    ContextManager context;
    context.updateBatteryLevel(90.0);
    context.updateTemperature(50.0);
    context.updateCPUUtilization(45.0);
    context.updateUserActivity(true);

    ContextAwareScheduler adaptiveScheduler;
    AdaptiveRoundRobin adaptiveRoundRobin;
    EDFScheduler edfScheduler;
    const std::vector<Process> realtimeWorkload =
        edfScheduler.loadRealtimeWorkload("data/realtime_workload.csv");
    const SchedulerResult adaptiveRoundRobinResult =
        adaptiveRoundRobin.schedule(workload, context);
    std::vector<ExperimentResult> results;
    results.push_back(fromSchedulerResult("FCFS", runFCFS(workload)));
    results.push_back(fromSchedulerResult("RoundRobin", runRoundRobin(workload, 4)));
    results.push_back(fromSchedulerResult(
        "Priority", runPriorityScheduling(workload)));
    results.push_back(fromSchedulerResult(
        "AdaptiveRoundRobin",
        adaptiveRoundRobinResult));
    results.push_back(fromSchedulerResult(
        "AdaptiveContextAware",
        adaptiveScheduler.schedule(workload, context)));
    const SchedulerResult edfResult = edfScheduler.schedule(realtimeWorkload);
    results.push_back(fromSchedulerResult("EDF", edfResult));

    std::cout << "\n====================================================\n";
    std::cout << "EXPERIMENT RESULTS\n";
    std::cout << "====================================================\n";
    displayResults(results);
    displaySummary(results);
    std::cout << "\n==========================\nAGING ANALYSIS\n==========================\n";
    std::cout << "Maximum Waiting Time      : "
              << adaptiveScheduler.getMaximumWaitingTime() << "\n";
    std::cout << "Average Aging Bonus       : "
              << adaptiveScheduler.getAverageAgingBonus() << "\n";
    std::cout << "Processes Rescued By Aging: "
              << adaptiveScheduler.getProcessesRescuedByAging() << "\n";
    std::filesystem::create_directories("results");
    adaptiveRoundRobin.exportResults(
        "results/adaptive_rr_results.csv",
        adaptiveRoundRobinResult);
    adaptiveRoundRobin.exportQuantumLog(
        "results/adaptive_rr_quantum_log.csv");
    edfScheduler.exportResults("results/edf_results.csv", edfResult);
    exportResults(results);
}

void ExperimentRunner::runAdaptiveDiagnostics(
    const std::vector<Process>& workload) {
    if (workload.empty()) {
        std::cerr << "Cannot diagnose an empty workload.\n";
        return;
    }

    ContextManager context;
    context.updateBatteryLevel(90.0);
    context.updateTemperature(50.0);
    context.updateCPUUtilization(45.0);
    context.updateUserActivity(true);

    ContextAwareScheduler adaptiveScheduler;
    const SchedulerResult adaptiveResult =
        adaptiveScheduler.schedule(workload, context);

    std::cout << "\n====================================================\n";
    std::cout << "ADAPTIVE SCHEDULER DIAGNOSTICS\n";
    std::cout << "====================================================\n";
    std::cout << "Context: battery=90, temperature=50, CPU=45, user=active\n";
    std::cout << "DynamicPriority = BasePriority + ContextScore / 10 "
                 "+ ProcessImpact / 10 + WaitingTime * 0.05\n";

    std::cout << "\nScheduling decision log:\n";
    std::cout << "Time | PID | Waiting Time | Aging Bonus | Dynamic Priority\n";
    std::cout << "--------------------------------------------------------\n";

    std::filesystem::create_directories("results");
    std::ofstream log("results/adaptive_decision_log.csv");
    log << "Time,ReadyQueue,SelectedProcess,WaitingTime,AgingBonus,"
           "DynamicPriority,Reason\n";
    log << std::fixed << std::setprecision(2);
    for (const auto& decision : adaptiveScheduler.getDecisionLog()) {
        std::ostringstream readyText;
        for (std::size_t i = 0; i < decision.readyQueue.size(); ++i) {
            if (i > 0) {
                readyText << " ";
            }
            readyText << "P" << decision.readyQueue[i];
        }
        const std::string reason = decision.rescuedByAging
            ? "Selected after aging changed the ranking"
            : "Highest dynamic priority among READY processes";
        std::cout << std::left << std::setw(7) << decision.time
                  << std::setw(6) << decision.pid
                  << std::setw(14) << decision.waitingTime
                  << std::setw(13) << decision.agingBonus
                  << decision.dynamicPriority << "\n";
        log << decision.time << ",\"" << readyText.str() << "\",P"
            << decision.pid << "," << decision.waitingTime << ","
            << decision.agingBonus << "," << decision.dynamicPriority
            << ",\"" << reason << "\"\n";
    }
    log.close();

    const SchedulerResult priorityResult = runPriorityScheduling(workload);
    std::cout << "\n==========================\nAGING ANALYSIS\n==========================\n";
    std::cout << "Maximum Waiting Time      : "
              << adaptiveScheduler.getMaximumWaitingTime() << "\n";
    std::cout << "Average Aging Bonus       : "
              << adaptiveScheduler.getAverageAgingBonus() << "\n";
    std::cout << "Processes Rescued By Aging: "
              << adaptiveScheduler.getProcessesRescuedByAging() << "\n";
    std::cout << "\nArrival-time handling comparison:\n";
    std::cout << "Priority Scheduler: selects the best priority among ready "
                 "processes and jumps only when the ready queue is empty.\n";
    std::cout << "Adaptive Scheduler: recalculates the choice from the "
                 "currently READY processes at every decision.\n";
    std::cout << "\nMetric comparison:\n";
    std::cout << "Priority average waiting time: "
              << priorityResult.avgWaitingTime << "\n";
    std::cout << "Adaptive average waiting time: "
              << adaptiveResult.avgWaitingTime << "\n";
    std::cout << "Priority total execution time: "
              << priorityResult.totalExecutionTime << "\n";
    std::cout << "Adaptive total execution time: "
              << adaptiveResult.totalExecutionTime << "\n";
    std::cout << "\nDiagnostic log exported to results/adaptive_decision_log.csv\n";
}

void ExperimentRunner::displayResults(
    const std::vector<ExperimentResult>& results) const {
    std::cout << std::left << std::setw(25) << "Scheduler"
              << std::setw(10) << "AvgWT"
              << std::setw(10) << "AvgTAT"
              << std::setw(10) << "AvgRT" << "\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.schedulerName
                  << std::setw(10) << result.avgWaitingTime
                  << std::setw(10) << result.avgTurnaroundTime
                  << std::setw(10) << result.avgResponseTime << "\n";
    }
}

void ExperimentRunner::displaySummary(
    const std::vector<ExperimentResult>& results) const {
    const auto bestWaiting = std::min_element(results.begin(), results.end(),
        [](const ExperimentResult& left, const ExperimentResult& right) {
            return left.avgWaitingTime < right.avgWaitingTime;
        });
    const auto bestTurnaround = std::min_element(results.begin(), results.end(),
        [](const ExperimentResult& left, const ExperimentResult& right) {
            return left.avgTurnaroundTime < right.avgTurnaroundTime;
        });
    const auto bestResponse = std::min_element(results.begin(), results.end(),
        [](const ExperimentResult& left, const ExperimentResult& right) {
            return left.avgResponseTime < right.avgResponseTime;
        });
    const auto bestThroughput = std::max_element(results.begin(), results.end(),
        [](const ExperimentResult& left, const ExperimentResult& right) {
            return left.throughput < right.throughput;
        });

    std::cout << "\n====================================================\nSUMMARY\n====================================================\n";
    std::cout << "Best Waiting Time      : " << bestWaiting->schedulerName << "\n";
    std::cout << "Best Turnaround Time   : " << bestTurnaround->schedulerName << "\n";
    std::cout << "Best Response Time     : " << bestResponse->schedulerName << "\n";
    std::cout << "Highest Throughput     : " << bestThroughput->schedulerName << "\n";
}

void ExperimentRunner::exportResults(
    const std::vector<ExperimentResult>& results) const {
    std::filesystem::create_directories("results");
    std::ofstream output("results/comparison_results.csv");
    if (!output.is_open()) {
        std::cerr << "Unable to write results/comparison_results.csv\n";
        return;
    }

    output << "Scheduler,AvgWaiting,AvgTurnaround,AvgResponse,Throughput,"
              "ContextSwitches,ExecutionTime,AverageWaitingTime,"
              "AverageTurnaroundTime,AverageResponseTime\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& result : results) {
        output << result.schedulerName << ","
               << result.avgWaitingTime << ","
               << result.avgTurnaroundTime << ","
               << result.avgResponseTime << ","
               << result.throughput << ","
               << result.contextSwitches << ","
               << result.totalExecutionTime << ","
               << result.avgWaitingTime << ","
               << result.avgTurnaroundTime << ","
               << result.avgResponseTime << "\n";
    }
    std::cout << "\nResults exported to results/comparison_results.csv\n";
}
