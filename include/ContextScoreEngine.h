#pragma once

#include "ContextManager.h"
#include "Process.h"

struct ProcessScore {
    int pid = 0;
    double score = 0.0;
};

class ContextScoreEngine {
public:
    double calculateContextScore(
        const Process& process,
        const ContextManager& context);

    void displayContextScoreBreakdown(
        const Process& process,
        const ContextManager& context);

private:
    struct ScoreBreakdown {
        double foregroundWeight;
        double batteryWeight;
        double temperatureWeight;
        double cpuUtilizationWeight;
        double userActivityWeight;
    };

    ScoreBreakdown calculateWeights(
        const Process& process,
        const ContextManager& context) const;
};
