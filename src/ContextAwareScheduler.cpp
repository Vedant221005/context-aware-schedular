#include "../include/ContextAwareScheduler.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>

namespace {

double normalizeImpact(double value) {
    return std::max(0.0, std::min(100.0, value));
}

}  // namespace

double ContextAwareScheduler::calculateProcessImpact(
    const Process& process,
    const ContextManager& context) const {
    const double batteryImpact = normalizeImpact(process.batteryImpact);
    const double temperatureImpact = normalizeImpact(process.temperatureImpact);
    const double cpuUsage = normalizeImpact(process.cpuUsage);

    double foregroundBonus = 0.0;
    double backgroundBonus = 0.0;
    if (context.userActive) {
        foregroundBonus = process.foregroundTask ? 20.0 : 0.0;
        backgroundBonus = process.backgroundTask ? 5.0 : 0.0;
    } else {
        foregroundBonus = process.foregroundTask ? 5.0 : 0.0;
        backgroundBonus = process.backgroundTask ? 15.0 : 0.0;
    }

    const double batteryPenalty = context.batteryLevel < 20.0
        ? batteryImpact * 0.5
        : 0.0;
    const double temperaturePenalty = context.cpuTemperature > 75.0
        ? temperatureImpact * 0.5
        : 0.0;
    const double cpuPenalty = context.cpuUtilization > 80.0
        ? cpuUsage * 0.5
        : 0.0;

    return foregroundBonus + backgroundBonus
        - batteryPenalty - temperaturePenalty - cpuPenalty;
}

SchedulerResult ContextAwareScheduler::schedule(
    std::vector<Process> processes,
    const ContextManager& context) {
    SchedulerResult result;
    result.schedulerName = "Adaptive Context-Aware Scheduler";
    dynamicProcesses.clear();
    executionOrder.clear();
    decisionLog.clear();
    maximumWaitingTime = 0;
    averageAgingBonus = 0.0;
    processesRescuedByAging = 0;

    ContextScoreEngine scoreEngine;
    for (const auto& process : processes) {
        const double score = scoreEngine.calculateContextScore(process, context);
        const double impact = calculateProcessImpact(process, context);
        dynamicProcesses.push_back({
            process,
            score,
            impact,
            process.getPriority() + score / 10.0 + impact / 10.0,
            0.0,
            process.getPriority() + score / 10.0 + impact / 10.0
        });
    }

    int currentTime = 0;
    int completedCount = 0;
    std::vector<bool> completed(dynamicProcesses.size(), false);
    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;
    double agingBonusSum = 0.0;

    while (completedCount < static_cast<int>(dynamicProcesses.size())) {
        std::vector<int> ready;
        for (int i = 0; i < static_cast<int>(dynamicProcesses.size()); ++i) {
            if (!completed[i]
                && dynamicProcesses[i].process.arrivalTime <= currentTime) {
                ready.push_back(i);
            }
        }

        if (ready.empty()) {
            int nextArrival = std::numeric_limits<int>::max();
            for (const auto& dynamicProcess : dynamicProcesses) {
                if (dynamicProcess.process.arrivalTime > currentTime) {
                    nextArrival = std::min(
                        nextArrival, dynamicProcess.process.arrivalTime);
                }
            }
            currentTime = nextArrival;
            continue;
        }

        for (const int index : ready) {
            DynamicProcess& dynamicProcess = dynamicProcesses[index];
            const int waitingTime = std::max(
                0, currentTime - dynamicProcess.process.arrivalTime);
            dynamicProcess.agingBonus = waitingTime * 0.05;
            dynamicProcess.dynamicPriority =
                dynamicProcess.baseDynamicPriority + dynamicProcess.agingBonus;
            maximumWaitingTime = std::max(maximumWaitingTime, waitingTime);
        }

        int selected = -1;
        for (const int i : ready) {
            const Process& candidate = dynamicProcesses[i].process;
            if (selected == -1
                || dynamicProcesses[i].dynamicPriority
                    > dynamicProcesses[selected].dynamicPriority
                || (dynamicProcesses[i].dynamicPriority
                        == dynamicProcesses[selected].dynamicPriority
                    && candidate.arrivalTime
                        < dynamicProcesses[selected].process.arrivalTime)
                || (dynamicProcesses[i].dynamicPriority
                        == dynamicProcesses[selected].dynamicPriority
                    && candidate.arrivalTime
                        == dynamicProcesses[selected].process.arrivalTime
                    && candidate.pid < dynamicProcesses[selected].process.pid)) {
                selected = i;
            }
        }

        int bestWithoutAging = ready.front();
        for (const int index : ready) {
            if (dynamicProcesses[index].baseDynamicPriority
                    > dynamicProcesses[bestWithoutAging].baseDynamicPriority
                || (dynamicProcesses[index].baseDynamicPriority
                        == dynamicProcesses[bestWithoutAging].baseDynamicPriority
                    && dynamicProcesses[index].process.arrivalTime
                        < dynamicProcesses[bestWithoutAging].process.arrivalTime)
                || (dynamicProcesses[index].baseDynamicPriority
                        == dynamicProcesses[bestWithoutAging].baseDynamicPriority
                    && dynamicProcesses[index].process.arrivalTime
                        == dynamicProcesses[bestWithoutAging].process.arrivalTime
                    && dynamicProcesses[index].process.pid
                        < dynamicProcesses[bestWithoutAging].process.pid)) {
                bestWithoutAging = index;
            }
        }

        const DynamicProcess& selectedDynamicProcess = dynamicProcesses[selected];
        const int waitingTime = std::max(
            0, currentTime - selectedDynamicProcess.process.arrivalTime);
        decisionLog.push_back({
            currentTime,
            selectedDynamicProcess.process.pid,
            waitingTime,
            selectedDynamicProcess.agingBonus,
            selectedDynamicProcess.dynamicPriority,
            selected != bestWithoutAging,
            [&ready, this]() {
                std::vector<int> pids;
                for (const int index : ready) {
                    pids.push_back(dynamicProcesses[index].process.pid);
                }
                return pids;
            }()
        });
        if (selected != bestWithoutAging) {
            ++processesRescuedByAging;
        }
        agingBonusSum += selectedDynamicProcess.agingBonus;

        const Process& process = dynamicProcesses[selected].process;
        ProcessStat stat;
        stat.pid = process.pid;
        stat.arrivalTime = process.arrivalTime;
        stat.burstTime = process.burstTime;
        stat.priority = process.priority;
        stat.startTime = currentTime;
        stat.completionTime = currentTime + process.burstTime;
        stat.waitingTime = stat.startTime - process.arrivalTime;
        stat.turnaroundTime = stat.completionTime - process.arrivalTime;
        stat.responseTime = stat.startTime - process.arrivalTime;

        result.processStats.push_back(stat);
        result.gantt.push_back({stat.pid, stat.startTime, stat.completionTime});
        executionOrder.push_back(stat.pid);
        waitingSum += stat.waitingTime;
        turnaroundSum += stat.turnaroundTime;
        responseSum += stat.responseTime;

        currentTime = stat.completionTime;
        completed[selected] = true;
        ++completedCount;
    }

    if (!result.processStats.empty()) {
        const double count = static_cast<double>(result.processStats.size());
        result.avgWaitingTime = waitingSum / count;
        result.avgTurnaroundTime = turnaroundSum / count;
        result.avgResponseTime = responseSum / count;
        result.throughput = count / std::max(1, currentTime);
    }

    result.contextSwitches = std::max(
        0, static_cast<int>(dynamicProcesses.size()) - 1);
    result.totalExecutionTime = currentTime;
    averageAgingBonus = result.processStats.empty()
        ? 0.0
        : agingBonusSum / static_cast<double>(result.processStats.size());
    return result;
}

void ContextAwareScheduler::displayExecutionOrder() const {
    std::cout << "\n====================================================\n";
    std::cout << "CONTEXT-AWARE SCHEDULER\n";
    std::cout << "====================================================\n";
    std::cout << "PID   Base   Context   Impact   Aging   Dynamic\n";
    std::cout << "----------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& dynamicProcess : dynamicProcesses) {
        std::cout << std::left << std::setw(6) << dynamicProcess.process.pid
                  << std::setw(7) << dynamicProcess.process.getPriority()
                  << std::setw(10) << dynamicProcess.contextScore
                  << std::setw(9) << dynamicProcess.processImpactScore
                  << std::setw(8) << dynamicProcess.agingBonus
                  << dynamicProcess.dynamicPriority << "\n";
    }

    std::cout << "\nExecution Order:\n";
    for (std::size_t i = 0; i < executionOrder.size(); ++i) {
        if (i > 0) {
            std::cout << " -> ";
        }
        std::cout << "P" << executionOrder[i];
    }
    std::cout << "\n";
    std::cout << "\n==========================\nAGING ANALYSIS\n==========================\n";
    std::cout << "Maximum Waiting Time      : " << maximumWaitingTime << "\n";
    std::cout << "Average Aging Bonus       : " << averageAgingBonus << "\n";
    std::cout << "Processes Rescued By Aging: "
              << processesRescuedByAging << "\n";
}

const std::vector<AdaptiveDecision>& ContextAwareScheduler::getDecisionLog() const {
    return decisionLog;
}

int ContextAwareScheduler::getMaximumWaitingTime() const {
    return maximumWaitingTime;
}

double ContextAwareScheduler::getAverageAgingBonus() const {
    return averageAgingBonus;
}

int ContextAwareScheduler::getProcessesRescuedByAging() const {
    return processesRescuedByAging;
}
