#pragma once

#include <string>
#include <vector>

#include "ContextManager.h"
#include "ContextTraceLoader.h"
#include "RealTimeContextMonitor.h"
#include "Process.h"
#include "SchedulerResult.h"

struct AdaptiveQuantumEvent {
    int time = 0;
    ContextSnapshot context;
    int quantum = 0;
    std::vector<int> readyQueue;
};

class AdaptiveRoundRobin {
public:
    SchedulerResult schedule(
        const std::vector<Process>& processes,
        const ContextManager& context);

    SchedulerResult schedule(
        const std::vector<Process>& processes,
        ContextManager context,
        const std::vector<ContextSnapshot>& contextTrace);

    SchedulerResult scheduleRealtime(
        const std::vector<Process>& processes,
        ContextManager context,
        RealTimeContextMonitor& monitor);

    int calculateAdaptiveQuantum(const ContextManager& context) const;
    const std::vector<AdaptiveQuantumEvent>& getQuantumLog() const;
    int getMinimumQuantum() const;
    int getMaximumQuantum() const;
    double getAverageQuantum() const;
    int getQuantumChangeCount() const;
    void displayDiagnostics() const;
    bool exportResults(const std::string& filename,
                       const SchedulerResult& result) const;
    bool exportQuantumLog(const std::string& filename) const;

private:
    SchedulerResult scheduleInternal(
        const std::vector<Process>& processes,
        ContextManager context,
        const std::vector<ContextSnapshot>* contextTrace,
        RealTimeContextMonitor* monitor = nullptr);

    std::vector<AdaptiveQuantumEvent> quantumLog;
    int minimumQuantum = 0;
    int maximumQuantum = 0;
    double averageQuantum = 0.0;
    int quantumChangeCount = 0;
};
