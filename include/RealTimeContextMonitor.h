#pragma once

#include <chrono>
#include <vector>

struct Context {
    double batteryLevel = 100.0;
    double cpuTemperature = -1.0;
    double cpuUtilization = 0.0;
    bool userActive = true;
    long long timestamp = 0;
};

class RealTimeContextMonitor {
public:
    explicit RealTimeContextMonitor(
        std::chrono::milliseconds refreshInterval =
            std::chrono::milliseconds(3000));

    Context sample();
    const Context& getLatest() const;
    const std::vector<Context>& getSamples() const;
    std::chrono::milliseconds getRefreshInterval() const;

private:
    Context collectContext();
    double readBatteryLevel() const;
    double readCPUUtilization();
    double readCPUTemperature() const;
    bool readUserActivity() const;

    std::chrono::milliseconds refreshInterval;
    Context latest;
    std::vector<Context> samples;
    bool hasCpuSample = false;
    unsigned long long previousIdle = 0;
    unsigned long long previousKernel = 0;
    unsigned long long previousUser = 0;
};
