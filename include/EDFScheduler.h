#pragma once

#include <string>
#include <vector>

#include "Process.h"
#include "RealTimeContextMonitor.h"
#include "SchedulerResult.h"

struct EDFDeadlineResult {
    int pid = 0;
    int deadline = 0;
    int finishTime = 0;
    bool met = false;
};

class EDFScheduler {
public:
    SchedulerResult schedule(const std::vector<Process>& processes);
    SchedulerResult scheduleRealtime(
        const std::vector<Process>& processes,
        RealTimeContextMonitor& monitor);
    std::vector<Process> loadRealtimeWorkload(const std::string& filename) const;
    void display(const SchedulerResult& result) const;
    bool exportResults(
        const std::string& filename,
        const SchedulerResult& result) const;
    const std::vector<EDFDeadlineResult>& getDeadlineResults() const;
    int getDeadlineMisses() const;

private:
    SchedulerResult scheduleInternal(
        const std::vector<Process>& processes,
        RealTimeContextMonitor* monitor);
    std::vector<EDFDeadlineResult> deadlineResults;
};
