#pragma once

#include <vector>

#include "ContextManager.h"
#include "ContextScoreEngine.h"
#include "ContextTraceLoader.h"
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

struct PriorityChange {
    int pid = 0;
    double oldPriority = 0.0;
    double newPriority = 0.0;
};

struct PriorityObservation {
    int pid = 0;
    double priority = 0.0;
};

struct ContextChange {
    ContextSnapshot snapshot;
    int appliedAt = 0;
    int runningPid = -1;
    std::vector<int> readyQueueBefore;
    std::vector<int> readyQueueAfter;
    std::vector<PriorityObservation> prioritiesBefore;
    std::vector<PriorityObservation> prioritiesAfter;
    std::vector<PriorityChange> priorityChanges;
};

class ContextAwareScheduler {
public:
    SchedulerResult schedule(
        std::vector<Process> processes,
        const ContextManager& context);

    SchedulerResult schedule(
        std::vector<Process> processes,
        ContextManager context,
        const std::vector<ContextSnapshot>& contextTrace);

    double calculateProcessImpact(
        const Process& process,
        const ContextManager& context) const;

    void displayExecutionOrder() const;
    const std::vector<AdaptiveDecision>& getDecisionLog() const;
    int getMaximumWaitingTime() const;
    double getAverageAgingBonus() const;
    int getProcessesRescuedByAging() const;
    const std::vector<ContextChange>& getContextChanges() const;
    void displayContextChanges() const;
    bool exportContextChanges(const std::string& filename) const;

private:
    SchedulerResult scheduleInternal(
        std::vector<Process> processes,
        ContextManager context,
        const std::vector<ContextSnapshot>* contextTrace);
    void applyContextSnapshot(
        const ContextSnapshot& snapshot,
        ContextManager& context,
        ContextScoreEngine& scoreEngine,
        const std::vector<bool>& completed,
        int appliedAt,
        int runningPid);

    std::vector<DynamicProcess> dynamicProcesses;
    std::vector<int> executionOrder;
    std::vector<AdaptiveDecision> decisionLog;
    int maximumWaitingTime = 0;
    double averageAgingBonus = 0.0;
    int processesRescuedByAging = 0;
    std::vector<ContextChange> contextChanges;
};
