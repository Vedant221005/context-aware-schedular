#pragma once

#include <iostream>

class ContextManager {
public:
    double batteryLevel;     // 0.0 - 100.0
    double cpuTemperature;   // degrees Celsius
    double cpuUtilization;   // 0.0 - 100.0
    bool userActive;

    ContextManager();

    // Simulated update methods (currently use placeholder/random values)
    void updateBatteryLevel(double simulatedValue);
    void updateTemperature(double simulatedValue);
    void updateCPUUtilization(double simulatedValue);
    void updateUserActivity(bool active);

    void displayContext() const;
};
