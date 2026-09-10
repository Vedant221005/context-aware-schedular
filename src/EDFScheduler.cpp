#include "../include/EDFScheduler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>

SchedulerResult EDFScheduler::schedule(const std::vector<Process>& processes) {
    return scheduleInternal(processes, nullptr);
}

SchedulerResult EDFScheduler::scheduleRealtime(
    const std::vector<Process>& processes,
    RealTimeContextMonitor& monitor) {
    return scheduleInternal(processes, &monitor);
}

SchedulerResult EDFScheduler::scheduleInternal(
    const std::vector<Process>& processes,
    RealTimeContextMonitor* monitor) {
    SchedulerResult result;
    result.schedulerName = "Earliest Deadline First";
    deadlineResults.clear();

    std::vector<Process> sorted = processes;
    std::sort(sorted.begin(), sorted.end(),
        [](const Process& left, const Process& right) {
            if (left.arrivalTime != right.arrivalTime) {
                return left.arrivalTime < right.arrivalTime;
            }
            return left.pid < right.pid;
        });
    std::vector<bool> completed(sorted.size(), false);
    int currentTime = 0;
    int completedCount = 0;
    int contextSwitches = 0;
    int lastPid = -1;
    bool hasLiveContext = false;
    Context lastContext;
    double waitingSum = 0.0;
    double turnaroundSum = 0.0;
    double responseSum = 0.0;

    while (completedCount < static_cast<int>(sorted.size())) {
        if (monitor != nullptr) {
            if (hasLiveContext) {
                std::this_thread::sleep_for(monitor->getRefreshInterval());
            }
            const Context currentContext = monitor->sample();
            const bool changed = !hasLiveContext
                || currentContext.batteryLevel != lastContext.batteryLevel
                || currentContext.cpuTemperature != lastContext.cpuTemperature
                || currentContext.cpuUtilization != lastContext.cpuUtilization
                || currentContext.userActive != lastContext.userActive;
            if (changed) {
                std::cout << "\nCONTEXT CHANGE DETECTED (EDF)\n"
                          << "Battery: " << currentContext.batteryLevel << "%\n"
                          << "CPU Usage: " << currentContext.cpuUtilization << "%\n"
                          << "Temperature: " << currentContext.cpuTemperature << "C\n"
                          << "User Active: "
                          << (currentContext.userActive ? "YES" : "NO") << "\n"
                          << "Deadline ordering remains unchanged.\n";
                lastContext = currentContext;
                hasLiveContext = true;
            }
        }
        std::vector<int> ready;
        for (int i = 0; i < static_cast<int>(sorted.size()); ++i) {
            if (!completed[i] && sorted[i].arrivalTime <= currentTime) {
                ready.push_back(i);
            }
        }
        if (ready.empty()) {
            int nextArrival = std::numeric_limits<int>::max();
            for (int i = 0; i < static_cast<int>(sorted.size()); ++i) {
                if (!completed[i]) {
                    nextArrival = std::min(nextArrival, sorted[i].arrivalTime);
                }
            }
            currentTime = nextArrival;
            continue;
        }

        const auto selected = std::min_element(ready.begin(), ready.end(),
            [&](int left, int right) {
                if (sorted[left].deadline != sorted[right].deadline) {
                    return sorted[left].deadline < sorted[right].deadline;
                }
                if (sorted[left].arrivalTime != sorted[right].arrivalTime) {
                    return sorted[left].arrivalTime < sorted[right].arrivalTime;
                }
                return sorted[left].pid < sorted[right].pid;
            });
        const int index = *selected;
        const Process& process = sorted[index];
        if (lastPid != -1 && lastPid != process.pid) {
            ++contextSwitches;
        }
        lastPid = process.pid;

        ProcessStat stat;
        stat.pid = process.pid;
        stat.arrivalTime = process.arrivalTime;
        stat.burstTime = process.burstTime;
        stat.priority = process.priority;
        stat.startTime = currentTime;
        stat.completionTime = currentTime + process.burstTime;
        stat.waitingTime = stat.startTime - process.arrivalTime;
        stat.turnaroundTime = stat.completionTime - process.arrivalTime;
        stat.responseTime = stat.waitingTime;
        result.processStats.push_back(stat);
        result.gantt.push_back({stat.pid, stat.startTime, stat.completionTime});
        deadlineResults.push_back({
            process.pid,
            process.deadline,
            stat.completionTime,
            stat.completionTime <= process.deadline
        });
        waitingSum += stat.waitingTime;
        turnaroundSum += stat.turnaroundTime;
        responseSum += stat.responseTime;
        currentTime = stat.completionTime;
        completed[index] = true;
        ++completedCount;
    }

    if (!result.processStats.empty()) {
        const double count = static_cast<double>(result.processStats.size());
        result.avgWaitingTime = waitingSum / count;
        result.avgTurnaroundTime = turnaroundSum / count;
        result.avgResponseTime = responseSum / count;
        result.throughput = count / std::max(1, currentTime);
    }
    result.contextSwitches = contextSwitches;
    result.totalExecutionTime = currentTime;
    return result;
}

std::vector<Process> EDFScheduler::loadRealtimeWorkload(
    const std::string& filename) const {
    std::vector<Process> processes;
    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Unable to open realtime workload: " << filename << "\n";
        return processes;
    }
    std::string line;
    bool firstLine = true;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (firstLine) {
            firstLine = false;
            if (line.find("PID") != std::string::npos) {
                continue;
            }
        }
        std::stringstream stream(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(stream, field, ',')) {
            fields.push_back(field);
        }
        if (fields.size() != 4) {
            continue;
        }
        try {
            Process process(std::stoi(fields[0]), std::stoi(fields[1]),
                std::stoi(fields[2]), 0);
            process.deadline = std::stoi(fields[3]);
            process.isRealTime = true;
            processes.push_back(process);
        } catch (const std::exception&) {
            std::cerr << "Skipping invalid realtime workload row.\n";
        }
    }
    return processes;
}

void EDFScheduler::display(const SchedulerResult& result) const {
    std::cout << "\n=================================\n";
    std::cout << "EARLIEST DEADLINE FIRST\n";
    std::cout << "=================================\n";
    std::cout << "PID  Arrival Burst Deadline\n";
    for (const auto& stat : result.processStats) {
        auto item = std::find_if(deadlineResults.begin(), deadlineResults.end(),
            [&](const EDFDeadlineResult& deadline) {
                return deadline.pid == stat.pid;
            });
        std::cout << stat.pid << "    " << stat.arrivalTime << "       "
                  << stat.burstTime << "      "
                  << (item == deadlineResults.end() ? 0 : item->deadline)
                  << "\n";
    }
    std::cout << "\nExecution Order:\n";
    for (std::size_t i = 0; i < result.gantt.size(); ++i) {
        if (i > 0) {
            std::cout << " -> ";
        }
        std::cout << "P" << result.gantt[i].pid;
    }
    std::cout << "\n\nDeadline Analysis:\n";
    std::cout << "PID  Deadline FinishTime Status\n";
    for (const auto& item : deadlineResults) {
        std::cout << item.pid << "    " << item.deadline << "       "
                  << item.finishTime << "       "
                  << (item.met ? "MET" : "MISSED") << "\n";
    }
    std::cout << "\nOverall Statistics:\n";
    std::cout << std::fixed << std::setprecision(2)
              << "Waiting Time      : " << result.avgWaitingTime << "\n"
              << "Turnaround Time   : " << result.avgTurnaroundTime << "\n"
              << "Response Time     : " << result.avgResponseTime << "\n"
              << "Throughput        : " << result.throughput << "\n"
              << "Context Switches  : " << result.contextSwitches << "\n"
              << "Total Execution Time: " << result.totalExecutionTime << "\n"
              << "Deadline Misses   : " << getDeadlineMisses() << "\n";
}

bool EDFScheduler::exportResults(
    const std::string& filename,
    const SchedulerResult& result) const {
    std::ofstream output(filename);
    if (!output.is_open()) {
        return false;
    }
    output << "Scheduler,AverageWaitingTime,AverageTurnaroundTime,"
              "AverageResponseTime,Throughput,ContextSwitches,ExecutionTime,"
              "DeadlineMisses\n";
    output << std::fixed << std::setprecision(4)
           << result.schedulerName << "," << result.avgWaitingTime << ","
           << result.avgTurnaroundTime << "," << result.avgResponseTime << ","
           << result.throughput << "," << result.contextSwitches << ","
           << result.totalExecutionTime << "," << getDeadlineMisses() << "\n";
    return true;
}

const std::vector<EDFDeadlineResult>& EDFScheduler::getDeadlineResults() const {
    return deadlineResults;
}

int EDFScheduler::getDeadlineMisses() const {
    return static_cast<int>(std::count_if(
        deadlineResults.begin(), deadlineResults.end(),
        [](const EDFDeadlineResult& result) { return !result.met; }));
}
