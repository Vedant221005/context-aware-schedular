#include "../include/EnergyModel.h"

#include <algorithm>

double EnergyModel::schedulerFactor(const std::string& schedulerName) {
    if (schedulerName == "FCFS") {
        return 1.00;
    }
    if (schedulerName == "Round Robin") {
        return 1.10;
    }
    if (schedulerName == "Priority") {
        return 0.95;
    }
    if (schedulerName == "Adaptive Context-Aware") {
        return 0.80;
    }
    if (schedulerName == "Adaptive Round Robin") {
        return 0.85;
    }
    if (schedulerName == "EDF") {
        return 0.90;
    }
    return 1.00;
}

double EnergyModel::calculateEnergy(
    double executionTime,
    int contextSwitches,
    double averageWaitingTime,
    double schedulerFactorValue) {
    const double switchOverhead = 1.0
        + static_cast<double>(std::max(0, contextSwitches)) / 100.0;
    const double waitingOverhead = 1.0
        + std::max(0.0, averageWaitingTime) / 1000.0;
    return std::max(0.0, executionTime)
        * schedulerFactorValue
        * switchOverhead
        * waitingOverhead;
}

double EnergyModel::calculatePower(
    const Process& process,
    const Context& context) {
    const double utilization = std::max(0.0, context.cpuUtilization);
    const double batteryAdjustment =
        std::max(0.0, 100.0 - context.batteryLevel) * 0.02;
    return 10.0
        + utilization * 0.5
        + batteryAdjustment
        + (process.foregroundTask ? 5.0 : 0.0);
}

double EnergyModel::calculateEnergy(
    double averagePower,
    double executionTime) {
    return averagePower * executionTime;
}
