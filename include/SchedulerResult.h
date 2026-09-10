#pragma once

#include <string>
#include <vector>

#include "Process.h"

struct ProcessStat {
    int pid = 0;
    int arrivalTime = 0;
    int burstTime = 0;
    int priority = 0;
    int startTime = 0;
    int completionTime = 0;
    int waitingTime = 0;
    int turnaroundTime = 0;
    int responseTime = 0;
};

struct GanttSegment {
    int pid = 0;
    int startTime = 0;
    int endTime = 0;
};

struct SchedulerResult {
    std::string schedulerName;
    std::vector<ProcessStat> processStats;
    std::vector<GanttSegment> gantt;
    double avgWaitingTime = 0.0;
    double avgTurnaroundTime = 0.0;
    double avgResponseTime = 0.0;
    double throughput = 0.0;
    int contextSwitches = 0;
    int totalExecutionTime = 0;
};
