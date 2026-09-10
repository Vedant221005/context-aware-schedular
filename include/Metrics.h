#pragma once

#include <iostream>

class Metrics {
public:
    double waitingTime;
    double turnaroundTime;
    double responseTime;
    double cpuUtilization;
    double throughput;
    int contextSwitches;
    double powerConsumption;
    double thermalEfficiency;

    Metrics();

    void displayMetrics() const;
};
