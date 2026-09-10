#pragma once

#include <string>
#include <vector>

#include "Process.h"

struct ExperimentResult {
    std::string schedulerName;
    double avgWaitingTime = 0.0;
    double avgTurnaroundTime = 0.0;
    double avgResponseTime = 0.0;
    double throughput = 0.0;
    int contextSwitches = 0;
    double totalExecutionTime = 0.0;
};

class ExperimentRunner {
public:
    void runExperiment(const std::vector<Process>& workload);
    void runAdaptiveDiagnostics(const std::vector<Process>& workload);

private:
    void exportResults(const std::vector<ExperimentResult>& results) const;
    void displayResults(const std::vector<ExperimentResult>& results) const;
    void displaySummary(const std::vector<ExperimentResult>& results) const;
};
