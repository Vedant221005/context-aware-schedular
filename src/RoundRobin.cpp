#include "../include/RoundRobin.h"

#include <algorithm>
#include <climits>
#include <queue>
#include <vector>

SchedulerResult runRoundRobin(const std::vector<Process>& processes, int quantum) {
    SchedulerResult result;
    result.schedulerName = "Round Robin";

    std::vector<Process> sorted = processes;
    std::sort(sorted.begin(), sorted.end(), [](const Process& a, const Process& b) {
        if (a.arrivalTime == b.arrivalTime) {
            return a.pid < b.pid;
        }
        return a.arrivalTime < b.arrivalTime;
    });

    const int processCount = static_cast<int>(sorted.size());
    if (processCount == 0) {
        return result;
    }

    std::vector<int> remainingBurst(processCount, 0);
    std::vector<int> completionTime(processCount, 0);
    std::vector<int> firstResponse(processCount, -1);
    std::vector<bool> arrived(processCount, false);
    std::queue<int> readyQueue;

    int currentTime = 0;
    int completed = 0;
    int lastScheduledPid = -1;
    int switchCount = 0;

    while (completed < processCount) {
        for (int i = 0; i < processCount; ++i) {
            if (!arrived[i] && sorted[i].arrivalTime <= currentTime) {
                arrived[i] = true;
                readyQueue.push(i);
                remainingBurst[i] = sorted[i].burstTime;
                if (firstResponse[i] == -1) {
                    firstResponse[i] = currentTime;
                }
            }
        }

        if (readyQueue.empty()) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < processCount; ++i) {
                if (!arrived[i]) {
                    nextArrival = std::min(nextArrival, sorted[i].arrivalTime);
                }
            }
            if (nextArrival == INT_MAX) {
                break;
            }
            currentTime = nextArrival;
            continue;
        }

        int idx = readyQueue.front();
        readyQueue.pop();

        if (lastScheduledPid != -1 && lastScheduledPid != sorted[idx].pid) {
            ++switchCount;
        }
        lastScheduledPid = sorted[idx].pid;

        const int slice = std::min(quantum, remainingBurst[idx]);
        const int startTime = currentTime;
        currentTime += slice;
        remainingBurst[idx] -= slice;

        result.gantt.push_back({sorted[idx].pid, startTime, currentTime});

        for (int i = 0; i < processCount; ++i) {
            if (!arrived[i] && sorted[i].arrivalTime <= currentTime) {
                arrived[i] = true;
                readyQueue.push(i);
                remainingBurst[i] = sorted[i].burstTime;
                if (firstResponse[i] == -1) {
                    firstResponse[i] = currentTime;
                }
            }
        }

        if (remainingBurst[idx] > 0) {
            readyQueue.push(idx);
        } else {
            completionTime[idx] = currentTime;
            ++completed;
        }
    }

    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;

    for (int i = 0; i < processCount; ++i) {
        ProcessStat stat;
        const Process& p = sorted[i];
        stat.pid = p.pid;
        stat.arrivalTime = p.arrivalTime;
        stat.burstTime = p.burstTime;
        stat.priority = p.priority;
        stat.startTime = firstResponse[i] == -1 ? p.arrivalTime : firstResponse[i];
        stat.completionTime = completionTime[i];
        stat.responseTime = std::max(0, firstResponse[i] - p.arrivalTime);
        stat.waitingTime = completionTime[i] - p.arrivalTime - p.burstTime;
        stat.turnaroundTime = completionTime[i] - p.arrivalTime;

        result.processStats.push_back(stat);
        waitingSum += stat.waitingTime;
        turnaroundSum += stat.turnaroundTime;
        responseSum += stat.responseTime;
    }

    if (processCount > 0) {
        result.avgWaitingTime = waitingSum / processCount;
        result.avgTurnaroundTime = turnaroundSum / processCount;
        result.avgResponseTime = responseSum / processCount;
        result.throughput = static_cast<double>(processCount) / std::max(1, currentTime);
    }

    result.contextSwitches = switchCount;
    result.totalExecutionTime = currentTime;
    return result;
}
