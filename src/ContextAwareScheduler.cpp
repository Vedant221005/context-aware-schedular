#include "../include/ContextAwareScheduler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <thread>

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
    return scheduleInternal(processes, context, nullptr);
}

SchedulerResult ContextAwareScheduler::schedule(
    std::vector<Process> processes,
    ContextManager context,
    const std::vector<ContextSnapshot>& contextTrace) {
    return scheduleInternal(processes, context, &contextTrace);
}

SchedulerResult ContextAwareScheduler::scheduleRealtime(
    std::vector<Process> processes,
    ContextManager context,
    RealTimeContextMonitor& monitor) {
    return scheduleInternal(processes, context, nullptr, &monitor);
}

SchedulerResult ContextAwareScheduler::scheduleInternal(
    std::vector<Process> processes,
    ContextManager context,
    const std::vector<ContextSnapshot>* contextTrace,
    RealTimeContextMonitor* monitor) {
    SchedulerResult result;
    result.schedulerName = "Adaptive Context-Aware Scheduler";
    dynamicProcesses.clear();
    executionOrder.clear();
    decisionLog.clear();
    maximumWaitingTime = 0;
    averageAgingBonus = 0.0;
    processesRescuedByAging = 0;
    contextChanges.clear();

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
    std::size_t nextContextUpdate = 0;
    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;
    double agingBonusSum = 0.0;
    bool hasLiveContext = false;
    Context lastLiveContext;

    while (completedCount < static_cast<int>(dynamicProcesses.size())) {
        if (monitor != nullptr) {
            if (hasLiveContext) {
                std::this_thread::sleep_for(monitor->getRefreshInterval());
            }
            const Context liveContext = monitor->sample();
            const bool changed = !hasLiveContext
                || liveContext.batteryLevel != lastLiveContext.batteryLevel
                || liveContext.cpuTemperature != lastLiveContext.cpuTemperature
                || liveContext.cpuUtilization != lastLiveContext.cpuUtilization
                || liveContext.userActive != lastLiveContext.userActive;
            if (changed) {
                std::cout << "\nCONTEXT CHANGE DETECTED\n"
                          << "Battery: " << liveContext.batteryLevel << "%\n"
                          << "CPU Usage: " << liveContext.cpuUtilization << "%\n"
                          << "Temperature: " << liveContext.cpuTemperature << "C\n"
                          << "User Active: "
                          << (liveContext.userActive ? "YES" : "NO") << "\n"
                          << "Scheduling priorities recalculated for ready processes.\n";
                ContextSnapshot snapshot{
                    currentTime,
                    liveContext.batteryLevel,
                    liveContext.cpuTemperature,
                    liveContext.cpuUtilization,
                    liveContext.userActive
                };
                applyContextSnapshot(
                    snapshot, context, scoreEngine, completed, currentTime, -1);
                lastLiveContext = liveContext;
                hasLiveContext = true;
            }
        }
        if (contextTrace != nullptr) {
            while (nextContextUpdate < contextTrace->size()
                && (*contextTrace)[nextContextUpdate].time <= currentTime) {
                applyContextSnapshot(
                    (*contextTrace)[nextContextUpdate],
                    context,
                    scoreEngine,
                    completed,
                    currentTime,
                    -1);
                ++nextContextUpdate;
            }
        }

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

        completed[selected] = true;
        currentTime = stat.completionTime;
        if (contextTrace != nullptr) {
            while (nextContextUpdate < contextTrace->size()
                && (*contextTrace)[nextContextUpdate].time <= currentTime) {
                applyContextSnapshot(
                    (*contextTrace)[nextContextUpdate],
                    context,
                    scoreEngine,
                    completed,
                    currentTime,
                    process.pid);
                ++nextContextUpdate;
            }
        }
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

void ContextAwareScheduler::applyContextSnapshot(
    const ContextSnapshot& snapshot,
    ContextManager& context,
    ContextScoreEngine& scoreEngine,
    const std::vector<bool>& completed,
    int appliedAt,
    int runningPid) {
    context.updateBatteryLevel(snapshot.battery);
    context.updateTemperature(snapshot.temperature);
    context.updateCPUUtilization(snapshot.cpuUtilization);
    context.updateUserActivity(snapshot.userActive);

    ContextChange change;
    change.snapshot = snapshot;
    change.appliedAt = appliedAt;
    change.runningPid = runningPid;
    for (std::size_t i = 0; i < dynamicProcesses.size(); ++i) {
        if (completed[i]) {
            continue;
        }

        DynamicProcess& dynamicProcess = dynamicProcesses[i];
        const double oldPriority = dynamicProcess.dynamicPriority;
        change.prioritiesBefore.push_back({
            dynamicProcess.process.pid,
            oldPriority
        });
        dynamicProcess.contextScore =
            scoreEngine.calculateContextScore(dynamicProcess.process, context);
        dynamicProcess.processImpactScore =
            calculateProcessImpact(dynamicProcess.process, context);
        dynamicProcess.baseDynamicPriority =
            dynamicProcess.process.getPriority()
            + dynamicProcess.contextScore / 10.0
            + dynamicProcess.processImpactScore / 10.0;
        const int waitingTime = std::max(
            0, snapshot.time - dynamicProcess.process.arrivalTime);
        dynamicProcess.agingBonus = waitingTime * 0.05;
        dynamicProcess.dynamicPriority =
            dynamicProcess.baseDynamicPriority + dynamicProcess.agingBonus;
        change.prioritiesAfter.push_back({
            dynamicProcess.process.pid,
            dynamicProcess.dynamicPriority
        });

        if (oldPriority != dynamicProcess.dynamicPriority) {
            change.priorityChanges.push_back({
                dynamicProcess.process.pid,
                oldPriority,
                dynamicProcess.dynamicPriority
            });
        }
    }
    auto orderByPriority = [](const std::vector<PriorityObservation>& values) {
        std::vector<PriorityObservation> ordered = values;
        std::sort(ordered.begin(), ordered.end(),
            [](const PriorityObservation& left, const PriorityObservation& right) {
                if (left.priority != right.priority) {
                    return left.priority > right.priority;
                }
                return left.pid < right.pid;
            });
        std::vector<int> pids;
        for (const auto& value : ordered) {
            pids.push_back(value.pid);
        }
        return pids;
    };
    change.readyQueueBefore = orderByPriority(change.prioritiesBefore);
    change.readyQueueAfter = orderByPriority(change.prioritiesAfter);
    contextChanges.push_back(change);
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

const std::vector<ContextChange>& ContextAwareScheduler::getContextChanges() const {
    return contextChanges;
}

void ContextAwareScheduler::displayContextChanges() const {
    for (const auto& change : contextChanges) {
        const ContextSnapshot& snapshot = change.snapshot;
        std::cout << "\n====================================================\n";
        std::cout << "CONTEXT UPDATE\n";
        std::cout << "====================================================\n";
        std::cout << "Time: " << snapshot.time << "\n\n";
        std::cout << "Battery: " << snapshot.battery << "%\n";
        std::cout << "Temperature: " << snapshot.temperature << "C\n";
        std::cout << "CPU Utilization: " << snapshot.cpuUtilization << "%\n";
        std::cout << "User Active: " << (snapshot.userActive ? "YES" : "NO")
                  << "\n";
        std::cout << "\nAffected Processes:\n";
        if (change.priorityChanges.empty()) {
            std::cout << "None\n";
        } else {
            std::cout << "PID   Previous Priority   New Priority\n";
            for (const auto& priorityChange : change.priorityChanges) {
                std::cout << std::left << std::setw(6) << priorityChange.pid
                          << std::setw(21) << priorityChange.oldPriority
                          << priorityChange.newPriority << "\n";
            }
        }
        std::cout << "====================================================\n";
    }
}

bool ContextAwareScheduler::exportContextChanges(
    const std::string& filename) const {
    std::ofstream output(filename);
    if (!output.is_open()) {
        std::cerr << "Unable to write context change log: " << filename << "\n";
        return false;
    }

    output << "Time,Battery,Temperature,CPUUtilization,UserActive\n";
    output << std::fixed << std::setprecision(2);
    for (const auto& change : contextChanges) {
        const ContextSnapshot& snapshot = change.snapshot;
        output << snapshot.time << "," << snapshot.battery << ","
               << snapshot.temperature << "," << snapshot.cpuUtilization
               << "," << (snapshot.userActive ? 1 : 0) << "\n";
    }
    return true;
}
