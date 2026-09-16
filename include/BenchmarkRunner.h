#pragma once

#include <string>
#include <vector>

#include "Process.h"
#include "SchedulerResult.h"

struct BenchmarkRecord {
    std::string workloadName;
    std::string schedulerName;
    double avgWaiting = 0.0;
    double avgTurnaround = 0.0;
    double avgResponse = 0.0;
    double throughput = 0.0;
    int contextSwitches = 0;
    double executionTime = 0.0;
    double averagePower = 0.0;
    double totalEnergy = 0.0;
    double averageTemperature = 0.0;
    double peakTemperature = 0.0;
    int thermalEvents = 0;
    double combinedScore = 0.0;
    double fgWaiting = 0.0;
    double bgWaiting = 0.0;
    double fgResponse = 0.0;
    double bgResponse = 0.0;
    double fgTurnaround = 0.0;
    double bgTurnaround = 0.0;
};

class BenchmarkRunner {
public:
    void run();

private:
    BenchmarkRecord analyze(
        const std::string& workloadName,
        const std::string& schedulerName,
        const std::vector<Process>& workload,
        const SchedulerResult& result) const;

    void exportBenchmarkResults(
        const std::vector<BenchmarkRecord>& records) const;
    void exportForegroundBackgroundResults(
        const std::vector<BenchmarkRecord>& records) const;
    void printComparison(const std::vector<BenchmarkRecord>& records) const;
    void printResearchSummary(
        const std::vector<BenchmarkRecord>& records) const;
};
