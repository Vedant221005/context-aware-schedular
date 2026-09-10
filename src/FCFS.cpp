#include "../include/FCFS.h"

#include <algorithm>
#include <numeric>
#include <vector>

SchedulerResult runFCFS(const std::vector<Process>& processes) {
    SchedulerResult result;
    result.schedulerName = "FCFS";

    std::vector<Process> sorted = processes;
    std::sort(sorted.begin(), sorted.end(), [](const Process& a, const Process& b) {
        if (a.arrivalTime == b.arrivalTime) {
            return a.pid < b.pid;
        }
        return a.arrivalTime < b.arrivalTime;
    });

    int currentTime = 0;
    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;

    for (const auto& process : sorted) {
        if (process.arrivalTime > currentTime) {
            currentTime = process.arrivalTime;
        }

        ProcessStat stat;
        stat.pid = process.pid;
        stat.arrivalTime = process.arrivalTime;
        stat.burstTime = process.burstTime;
        stat.priority = process.priority;
        stat.startTime = currentTime;
        stat.completionTime = currentTime + process.burstTime;
        stat.waitingTime = stat.completionTime - process.burstTime - process.arrivalTime;
        stat.turnaroundTime = stat.completionTime - process.arrivalTime;
        stat.responseTime = stat.startTime - process.arrivalTime;

        result.processStats.push_back(stat);
        result.gantt.push_back({stat.pid, stat.startTime, stat.completionTime});

        currentTime = stat.completionTime;
        waitingSum += stat.waitingTime;
        turnaroundSum += stat.turnaroundTime;
        responseSum += stat.responseTime;
    }

    const int processCount = static_cast<int>(sorted.size());
    if (processCount > 0) {
        result.avgWaitingTime = waitingSum / processCount;
        result.avgTurnaroundTime = turnaroundSum / processCount;
        result.avgResponseTime = responseSum / processCount;
        result.throughput = static_cast<double>(processCount) / std::max(1, currentTime);
    }

    result.contextSwitches = std::max(0, processCount - 1);
    result.totalExecutionTime = currentTime;
    return result;
}
