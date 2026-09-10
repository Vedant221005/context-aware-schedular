#include "../include/PriorityScheduler.h"

#include <algorithm>
#include <climits>
#include <vector>

SchedulerResult runPriorityScheduling(const std::vector<Process>& processes) {
    SchedulerResult result;
    result.schedulerName = "Non-Preemptive Priority";

    std::vector<Process> sorted = processes;
    std::sort(sorted.begin(), sorted.end(), [](const Process& a, const Process& b) {
        if (a.arrivalTime == b.arrivalTime) {
            return a.priority < b.priority;
        }
        return a.arrivalTime < b.arrivalTime;
    });

    std::vector<bool> completed(sorted.size(), false);
    std::vector<ProcessStat> stats;
    int currentTime = 0;

    while (std::any_of(completed.begin(), completed.end(), [](bool done) { return !done; })) {
        std::vector<int> ready;
        for (int i = 0; i < static_cast<int>(sorted.size()); ++i) {
            if (!completed[i] && sorted[i].arrivalTime <= currentTime) {
                ready.push_back(i);
            }
        }

        if (ready.empty()) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < static_cast<int>(sorted.size()); ++i) {
                if (!completed[i]) {
                    nextArrival = std::min(nextArrival, sorted[i].arrivalTime);
                }
            }
            if (nextArrival == INT_MAX) {
                break;
            }
            currentTime = nextArrival;
            continue;
        }

        std::sort(ready.begin(), ready.end(), [&](int a, int b) {
            if (sorted[a].priority == sorted[b].priority) {
                return sorted[a].arrivalTime < sorted[b].arrivalTime;
            }
            return sorted[a].priority < sorted[b].priority;
        });

        int idx = ready.front();
        const Process& process = sorted[idx];

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

        stats.push_back(stat);
        result.gantt.push_back({stat.pid, stat.startTime, stat.completionTime});

        currentTime = stat.completionTime;
        completed[idx] = true;
    }

    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;
    for (const auto& stat : stats) {
        waitingSum += stat.waitingTime;
        turnaroundSum += stat.turnaroundTime;
        responseSum += stat.responseTime;
    }

    if (!stats.empty()) {
        result.avgWaitingTime = waitingSum / stats.size();
        result.avgTurnaroundTime = turnaroundSum / stats.size();
        result.avgResponseTime = responseSum / stats.size();
        result.throughput = static_cast<double>(stats.size()) / std::max(1, currentTime);
    }

    result.contextSwitches = std::max(0, static_cast<int>(stats.size()) - 1);
    result.processStats = stats;
    result.totalExecutionTime = currentTime;
    return result;
}
