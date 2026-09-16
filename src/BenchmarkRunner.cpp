#include "../include/BenchmarkRunner.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>

#include "../include/AdaptiveRoundRobin.h"
#include "../include/ContextAwareScheduler.h"
#include "../include/ContextManager.h"
#include "../include/DatasetLoader.h"
#include "../include/EDFScheduler.h"
#include "../include/EnergyModel.h"
#include "../include/FCFS.h"
#include "../include/PriorityScheduler.h"
#include "../include/RoundRobin.h"
#include "../include/ThermalModel.h"

namespace {

Context benchmarkContext() {
    Context context;
    context.batteryLevel = 20.0;
    context.cpuTemperature = 70.0;
    context.cpuUtilization = 50.0;
    context.userActive = true;
    return context;
}

ContextManager benchmarkContextManager() {
    ContextManager context;
    context.updateBatteryLevel(20.0);
    context.updateTemperature(70.0);
    context.updateCPUUtilization(50.0);
    context.updateUserActivity(true);
    return context;
}

template <typename T>
double averageOrZero(const std::vector<T>& values) {
    if (values.empty()) {
        return 0.0;
    }
    return std::accumulate(values.begin(), values.end(), 0.0)
        / static_cast<double>(values.size());
}

std::string csvEscape(const std::string& value) {
    std::ostringstream output;
    output << '"' << value << '"';
    return output.str();
}

}  // namespace

void BenchmarkRunner::run() {
    DatasetLoader loader;
    const std::vector<std::string> workloadNames = {
        "workload_50.csv", "workload_100.csv", "workload_200.csv"
    };
    std::vector<BenchmarkRecord> records;

    for (const auto& workloadName : workloadNames) {
        const std::vector<Process> workload =
            loader.loadCSV("data/" + workloadName);
        if (workload.empty()) {
            std::cerr << "Benchmark skipped empty workload: "
                      << workloadName << "\n";
            continue;
        }

        const ContextManager context = benchmarkContextManager();
        ContextAwareScheduler contextAware;
        AdaptiveRoundRobin adaptiveRoundRobin;
        EDFScheduler edf;
        const std::vector<std::pair<std::string, SchedulerResult>> results = {
            {"FCFS", runFCFS(workload)},
            {"Round Robin", runRoundRobin(workload, 4)},
            {"Priority", runPriorityScheduling(workload)},
            {"Adaptive Context-Aware", contextAware.schedule(workload, context)},
            {"Adaptive Round Robin", adaptiveRoundRobin.schedule(workload, context)},
            {"EDF", edf.schedule(workload)}
        };

        for (const auto& item : results) {
            records.push_back(analyze(
                workloadName, item.first, workload, item.second));
        }
    }

    exportBenchmarkResults(records);
    exportForegroundBackgroundResults(records);
    printComparison(records);
    printResearchSummary(records);
}

BenchmarkRecord BenchmarkRunner::analyze(
    const std::string& workloadName,
    const std::string& schedulerName,
    const std::vector<Process>& workload,
    const SchedulerResult& result) const {
    BenchmarkRecord record;
    record.workloadName = workloadName;
    record.schedulerName = schedulerName;
    record.avgWaiting = result.avgWaitingTime;
    record.avgTurnaround = result.avgTurnaroundTime;
    record.avgResponse = result.avgResponseTime;
    record.throughput = result.throughput;
    record.contextSwitches = result.contextSwitches;
    record.executionTime = result.totalExecutionTime;

    const double factor = EnergyModel::schedulerFactor(schedulerName);
    record.totalEnergy = EnergyModel::calculateEnergy(
        record.executionTime,
        record.contextSwitches,
        record.avgWaiting,
        factor);
    record.averagePower = record.executionTime > 0.0
        ? record.totalEnergy / record.executionTime : 0.0;
    const ThermalModel::Simulation thermal = ThermalModel::simulate(
        record.executionTime, record.contextSwitches, factor);
    record.averageTemperature = thermal.averageTemperature;
    record.peakTemperature = thermal.peakTemperature;
    record.thermalEvents = thermal.thermalEvents;
    record.combinedScore = 0.40 * record.avgResponse
        + 0.30 * record.totalEnergy
        + 0.30 * record.peakTemperature;

    std::vector<double> fgWaiting;
    std::vector<double> bgWaiting;
    std::vector<double> fgResponse;
    std::vector<double> bgResponse;
    std::vector<double> fgTurnaround;
    std::vector<double> bgTurnaround;
    for (const auto& stat : result.processStats) {
        const auto process = std::find_if(
            workload.begin(), workload.end(),
            [&](const Process& candidate) { return candidate.pid == stat.pid; });
        if (process == workload.end()) {
            continue;
        }
        if (process->foregroundTask) {
            fgWaiting.push_back(stat.waitingTime);
            fgResponse.push_back(stat.responseTime);
            fgTurnaround.push_back(stat.turnaroundTime);
        } else {
            bgWaiting.push_back(stat.waitingTime);
            bgResponse.push_back(stat.responseTime);
            bgTurnaround.push_back(stat.turnaroundTime);
        }
    }
    record.fgWaiting = averageOrZero(fgWaiting);
    record.bgWaiting = averageOrZero(bgWaiting);
    record.fgResponse = averageOrZero(fgResponse);
    record.bgResponse = averageOrZero(bgResponse);
    record.fgTurnaround = averageOrZero(fgTurnaround);
    record.bgTurnaround = averageOrZero(bgTurnaround);
    return record;
}

void BenchmarkRunner::exportBenchmarkResults(
    const std::vector<BenchmarkRecord>& records) const {
    std::filesystem::create_directories("results");
    std::ofstream output("results/benchmark_results.csv");
    if (!output.is_open()) {
        std::cerr << "Unable to write benchmark_results.csv.\n";
        return;
    }
    output << "Scheduler,AvgWaiting,AvgTurnaround,AvgResponse,Throughput,"
              "ContextSwitches,EnergyScore,AverageTemperature,"
              "PeakTemperature,ThermalEvents,CombinedScore\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& record : records) {
        output << csvEscape(record.schedulerName) << "," << record.avgWaiting
               << "," << record.avgTurnaround << "," << record.avgResponse
               << "," << record.throughput << "," << record.contextSwitches
               << "," << record.totalEnergy << ","
               << record.averageTemperature << "," << record.peakTemperature
               << "," << record.thermalEvents << ","
               << record.combinedScore << "\n";
    }
}

void BenchmarkRunner::exportForegroundBackgroundResults(
    const std::vector<BenchmarkRecord>& records) const {
    std::filesystem::create_directories("results");
    std::ofstream output("results/fg_bg_analysis.csv");
    if (!output.is_open()) {
        std::cerr << "Unable to write fg_bg_analysis.csv.\n";
        return;
    }
    output << "Scheduler,FGWaiting,BGWaiting,FGResponse,BGResponse,"
              "FGTurnaround,BGTurnaround\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& record : records) {
        output << csvEscape(record.schedulerName) << "," << record.fgWaiting
               << "," << record.bgWaiting << "," << record.fgResponse << ","
               << record.bgResponse << "," << record.fgTurnaround << ","
               << record.bgTurnaround << "\n";
    }
}

void BenchmarkRunner::printComparison(
    const std::vector<BenchmarkRecord>& records) const {
    std::cout << "\n====================================================\n"
                 "BENCHMARK ANALYSIS\n"
                 "====================================================\n";
    std::cout << std::left << std::setw(20) << "Workload"
              << std::setw(24) << "Scheduler"
              << std::setw(12) << "AvgResponse"
              << std::setw(12) << "Energy"
              << std::setw(12) << "PeakTemp"
              << "CombinedScore\n";
    for (const auto& record : records) {
        std::cout << std::left << std::setw(20) << record.workloadName
                  << std::setw(24) << record.schedulerName
                  << std::setw(12) << std::fixed << std::setprecision(2)
                  << record.avgResponse << std::setw(12) << record.totalEnergy
                  << std::setw(12) << record.peakTemperature
                  << record.combinedScore << "\n";
    }
    std::cout << "\nReports exported to results/benchmark_results.csv and "
                 "results/fg_bg_analysis.csv\n";
}

void BenchmarkRunner::printResearchSummary(
    const std::vector<BenchmarkRecord>& records) const {
    if (records.empty()) {
        std::cout << "\nNo benchmark records were produced.\n";
        return;
    }

    std::map<std::string, BenchmarkRecord> aggregate;
    std::map<std::string, int> counts;
    for (const auto& record : records) {
        BenchmarkRecord& summary = aggregate[record.schedulerName];
        summary.schedulerName = record.schedulerName;
        summary.avgResponse += record.avgResponse;
        summary.totalEnergy += record.totalEnergy;
        summary.thermalEvents += record.thermalEvents;
        summary.combinedScore += record.combinedScore;
        ++counts[record.schedulerName];
    }
    std::vector<BenchmarkRecord> summaries;
    for (auto& entry : aggregate) {
        const double count = static_cast<double>(counts[entry.first]);
        entry.second.avgResponse /= count;
        entry.second.totalEnergy /= count;
        entry.second.thermalEvents =
            static_cast<int>(entry.second.thermalEvents / count);
        entry.second.combinedScore /= count;
        summaries.push_back(entry.second);
    }

    const auto bestBy = [&](auto selector) {
        return std::min_element(
            summaries.begin(), summaries.end(),
            [&](const BenchmarkRecord& left, const BenchmarkRecord& right) {
                return selector(left) < selector(right);
            });
    };
    const auto bestResponse = bestBy(
        [](const BenchmarkRecord& record) { return record.avgResponse; });
    const auto bestEnergy = bestBy(
        [](const BenchmarkRecord& record) { return record.totalEnergy; });
    const auto bestThermal = bestBy(
        [](const BenchmarkRecord& record) { return record.thermalEvents; });
    const auto bestOverall = bestBy(
        [](const BenchmarkRecord& record) { return record.combinedScore; });

    std::cout << "\n====================================================\n"
                 "RESEARCH SUMMARY\n"
                 "====================================================\n"
              << "Best Overall Scheduler: " << bestOverall->schedulerName << "\n"
              << "Best Energy Efficient Scheduler: " << bestEnergy->schedulerName
              << "\n"
              << "Best Thermal Scheduler: " << bestThermal->schedulerName << "\n"
              << "Best Responsive Scheduler: " << bestResponse->schedulerName
              << "\n";
}
