#pragma once

#include <vector>

#include "ContextManager.h"
#include "ContextScoreEngine.h"
#include "Process.h"
#include "SchedulerResult.h"

struct DynamicProcess {
    Process process;
    double contextScore = 0.0;
    double processImpactScore = 0.0;
    double baseDynamicPriority = 0.0;
    double agingBonus = 0.0;
    double dynamicPriority = 0.0;
};

struct AdaptiveDecision {
    int time = 0;
    int pid = 0;
    int waitingTime = 0;
    double agingBonus = 0.0;
    double dynamicPriority = 0.0;
    bool rescuedByAging = false;
    std::vector<int> readyQueue;
};

class ContextAwareScheduler {
public:
    SchedulerResult schedule(
        std::vector<Process> processes,
        const ContextManager& context);

    double calculateProcessImpact(
        const Process& process,
        const ContextManager& context) const;

    void displayExecutionOrder() const;
    const std::vector<AdaptiveDecision>& getDecisionLog() const;
    int getMaximumWaitingTime() const;
    double getAverageAgingBonus() const;
    int getProcessesRescuedByAging() const;

private:
    std::vector<DynamicProcess> dynamicProcesses;
    std::vector<int> executionOrder;
    std::vector<AdaptiveDecision> decisionLog;
    int maximumWaitingTime = 0;
    double averageAgingBonus = 0.0;
    int processesRescuedByAging = 0;
};
