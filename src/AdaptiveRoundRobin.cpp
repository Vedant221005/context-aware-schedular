#include "../include/AdaptiveRoundRobin.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <thread>

namespace {

int clampQuantum(int quantum) {
    return std::max(2, std::min(20, quantum));
}

void applySnapshot(const ContextSnapshot& snapshot, ContextManager& context) {
    context.updateBatteryLevel(snapshot.battery);
    context.updateTemperature(snapshot.temperature);
    context.updateCPUUtilization(snapshot.cpuUtilization);
    context.updateUserActivity(snapshot.userActive);
}

}  // namespace

int AdaptiveRoundRobin::calculateAdaptiveQuantum(
    const ContextManager& context) const {
    int quantum = 6;
    if (context.batteryLevel > 80.0) {
        quantum -= 2;
    } else if (context.batteryLevel >= 30.0
        && context.batteryLevel < 50.0) {
        quantum += 2;
    } else if (context.batteryLevel < 30.0) {
        quantum += 6;
    }

    if (context.cpuTemperature > 80.0) {
        quantum += 3;
    } else if (context.cpuTemperature >= 60.0) {
        quantum += 1;
    }

    if (context.cpuUtilization > 80.0) {
        quantum += 3;
    } else if (context.cpuUtilization >= 60.0) {
        quantum += 1;
    }

    quantum += context.userActive ? -1 : 2;
    return clampQuantum(quantum);
}

SchedulerResult AdaptiveRoundRobin::schedule(
    const std::vector<Process>& processes,
    const ContextManager& context) {
    return scheduleInternal(processes, context, nullptr);
}

SchedulerResult AdaptiveRoundRobin::schedule(
    const std::vector<Process>& processes,
    ContextManager context,
    const std::vector<ContextSnapshot>& contextTrace) {
    return scheduleInternal(processes, context, &contextTrace);
}

SchedulerResult AdaptiveRoundRobin::scheduleRealtime(
    const std::vector<Process>& processes,
    ContextManager context,
    RealTimeContextMonitor& monitor) {
    return scheduleInternal(processes, context, nullptr, &monitor);
}

SchedulerResult AdaptiveRoundRobin::scheduleInternal(
    const std::vector<Process>& processes,
    ContextManager context,
    const std::vector<ContextSnapshot>* contextTrace,
    RealTimeContextMonitor* monitor) {
    SchedulerResult result;
    result.schedulerName = "Adaptive Round Robin";
    quantumLog.clear();
    minimumQuantum = 0;
    maximumQuantum = 0;
    averageQuantum = 0.0;
    quantumChangeCount = 0;

    std::vector<Process> sorted = processes;
    std::sort(sorted.begin(), sorted.end(),
        [](const Process& left, const Process& right) {
            if (left.arrivalTime != right.arrivalTime) {
                return left.arrivalTime < right.arrivalTime;
            }
            return left.pid < right.pid;
        });
    const int count = static_cast<int>(sorted.size());
    if (count == 0) {
        return result;
    }

    std::vector<int> remaining(count);
    std::vector<int> firstStart(count, -1);
    std::vector<int> completion(count, 0);
    std::vector<bool> arrived(count, false);
    std::vector<bool> finished(count, false);
    std::queue<int> readyQueue;
    int currentTime = 0;
    int completedCount = 0;
    int lastPid = -1;
    int contextSwitches = 0;
    std::size_t nextTrace = 0;
    int currentQuantum = calculateAdaptiveQuantum(context);
    double quantumSum = 0.0;
    bool hasLiveContext = false;
    Context lastLiveContext;

    auto recordQuantum = [&](int time) {
        const int newQuantum = calculateAdaptiveQuantum(context);
        if (quantumLog.empty() || newQuantum != currentQuantum) {
            if (!quantumLog.empty()) {
                ++quantumChangeCount;
            }
            currentQuantum = newQuantum;
        }
        std::vector<int> queued;
        std::queue<int> copy = readyQueue;
        while (!copy.empty()) {
            queued.push_back(sorted[copy.front()].pid);
            copy.pop();
        }
        ContextSnapshot snapshot{
            time,
            context.batteryLevel,
            context.cpuTemperature,
            context.cpuUtilization,
            context.userActive
        };
        quantumLog.push_back({time, snapshot, currentQuantum, queued});
        minimumQuantum = minimumQuantum == 0
            ? currentQuantum : std::min(minimumQuantum, currentQuantum);
        maximumQuantum = std::max(maximumQuantum, currentQuantum);
        quantumSum += currentQuantum;
    };

    auto applyPendingTrace = [&]() {
        if (contextTrace == nullptr) {
            return;
        }
        while (nextTrace < contextTrace->size()
            && (*contextTrace)[nextTrace].time <= currentTime) {
            applySnapshot((*contextTrace)[nextTrace], context);
            recordQuantum((*contextTrace)[nextTrace].time);
            ++nextTrace;
        }
    };

    while (completedCount < count) {
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
                context.updateBatteryLevel(liveContext.batteryLevel);
                context.updateTemperature(liveContext.cpuTemperature);
                context.updateCPUUtilization(liveContext.cpuUtilization);
                context.updateUserActivity(liveContext.userActive);
                lastLiveContext = liveContext;
                hasLiveContext = true;
                recordQuantum(currentTime);
            }
        }
        applyPendingTrace();
        for (int i = 0; i < count; ++i) {
            if (!arrived[i] && sorted[i].arrivalTime <= currentTime) {
                arrived[i] = true;
                remaining[i] = sorted[i].burstTime;
                readyQueue.push(i);
            }
        }
        if (readyQueue.empty()) {
            int nextArrival = std::numeric_limits<int>::max();
            for (int i = 0; i < count; ++i) {
                if (!arrived[i]) {
                    nextArrival = std::min(nextArrival, sorted[i].arrivalTime);
                }
            }
            currentTime = nextArrival;
            continue;
        }

        if (quantumLog.empty()) {
            recordQuantum(currentTime);
        }
        const int index = readyQueue.front();
        readyQueue.pop();
        if (lastPid != -1 && lastPid != sorted[index].pid) {
            ++contextSwitches;
        }
        lastPid = sorted[index].pid;
        if (firstStart[index] == -1) {
            firstStart[index] = currentTime;
        }
        const int slice = std::min(currentQuantum, remaining[index]);
        const int start = currentTime;
        currentTime += slice;
        remaining[index] -= slice;
        result.gantt.push_back({sorted[index].pid, start, currentTime});

        for (int i = 0; i < count; ++i) {
            if (!arrived[i] && sorted[i].arrivalTime <= currentTime) {
                arrived[i] = true;
                remaining[i] = sorted[i].burstTime;
                readyQueue.push(i);
            }
        }
        applyPendingTrace();
        if (remaining[index] > 0) {
            readyQueue.push(index);
        } else {
            finished[index] = true;
            completion[index] = currentTime;
            ++completedCount;
        }
    }

    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;
    for (int i = 0; i < count; ++i) {
        ProcessStat stat;
        stat.pid = sorted[i].pid;
        stat.arrivalTime = sorted[i].arrivalTime;
        stat.burstTime = sorted[i].burstTime;
        stat.priority = sorted[i].priority;
        stat.startTime = firstStart[i];
        stat.completionTime = completion[i];
        stat.turnaroundTime = completion[i] - sorted[i].arrivalTime;
        stat.waitingTime = stat.turnaroundTime - sorted[i].burstTime;
        stat.responseTime = firstStart[i] - sorted[i].arrivalTime;
        result.processStats.push_back(stat);
        waitingSum += stat.waitingTime;
        turnaroundSum += stat.turnaroundTime;
        responseSum += stat.responseTime;
    }
    const double processCount = static_cast<double>(count);
    result.avgWaitingTime = waitingSum / processCount;
    result.avgTurnaroundTime = turnaroundSum / processCount;
    result.avgResponseTime = responseSum / processCount;
    result.throughput = processCount / std::max(1, currentTime);
    result.contextSwitches = contextSwitches;
    result.totalExecutionTime = currentTime;
    averageQuantum = quantumLog.empty()
        ? 0.0 : quantumSum / static_cast<double>(quantumLog.size());
    return result;
}

const std::vector<AdaptiveQuantumEvent>& AdaptiveRoundRobin::getQuantumLog() const {
    return quantumLog;
}

int AdaptiveRoundRobin::getMinimumQuantum() const { return minimumQuantum; }
int AdaptiveRoundRobin::getMaximumQuantum() const { return maximumQuantum; }
double AdaptiveRoundRobin::getAverageQuantum() const { return averageQuantum; }
int AdaptiveRoundRobin::getQuantumChangeCount() const {
    return quantumChangeCount;
}

void AdaptiveRoundRobin::displayDiagnostics() const {
    std::cout << "\n====================================================\n";
    std::cout << "ADAPTIVE ROUND ROBIN\n";
    std::cout << "====================================================\n";
    std::cout << "Time | Battery | Temp | CPU | User | Quantum\n";
    for (const auto& event : quantumLog) {
        std::cout << event.time << " | " << event.context.battery << " | "
                  << event.context.temperature << " | "
                  << event.context.cpuUtilization << " | "
                  << (event.context.userActive ? "YES" : "NO") << " | "
                  << event.quantum << "\n";
        std::cout << "Ready Queue: ";
        for (const int pid : event.readyQueue) {
            std::cout << "P" << pid << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n====================================================\n";
    std::cout << "QUANTUM ANALYSIS\n";
    std::cout << "====================================================\n";
    std::cout << "Minimum Quantum Used : " << minimumQuantum << "\n";
    std::cout << "Maximum Quantum Used : " << maximumQuantum << "\n";
    std::cout << "Average Quantum Used : " << averageQuantum << "\n";
    std::cout << "Number Of Quantum Changes : " << quantumChangeCount << "\n";
}

bool AdaptiveRoundRobin::exportResults(
    const std::string& filename,
    const SchedulerResult& result) const {
    std::ofstream output(filename);
    if (!output.is_open()) {
        std::cerr << "Unable to write Adaptive Round Robin results: "
                  << filename << "\n";
        return false;
    }
    output << "Scheduler,AverageWaitingTime,AverageTurnaroundTime,"
              "AverageResponseTime,Throughput,ContextSwitches,ExecutionTime\n";
    output << std::fixed << std::setprecision(4)
           << result.schedulerName << "," << result.avgWaitingTime << ","
           << result.avgTurnaroundTime << "," << result.avgResponseTime << ","
           << result.throughput << "," << result.contextSwitches << ","
           << result.totalExecutionTime << "\n";
    return true;
}

bool AdaptiveRoundRobin::exportQuantumLog(const std::string& filename) const {
    std::ofstream output(filename);
    if (!output.is_open()) {
        std::cerr << "Unable to write Adaptive Round Robin quantum log: "
                  << filename << "\n";
        return false;
    }
    output << "Time,Battery,Temperature,CPUUtilization,UserActive,Quantum\n";
    output << std::fixed << std::setprecision(2);
    for (const auto& event : quantumLog) {
        output << event.time << "," << event.context.battery << ","
               << event.context.temperature << ","
               << event.context.cpuUtilization << ","
               << (event.context.userActive ? 1 : 0) << ","
               << event.quantum << "\n";
    }
    return true;
}
