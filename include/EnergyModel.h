#pragma once

#include <string>

#include "Process.h"
#include "RealTimeContextMonitor.h"

class EnergyModel {
public:
    static double schedulerFactor(const std::string& schedulerName);

    static double calculateEnergy(
        double executionTime,
        int contextSwitches,
        double averageWaitingTime,
        double schedulerFactor);

    static double calculatePower(
        const Process& process,
        const Context& context);

    static double calculateEnergy(double averagePower, double executionTime);
};
