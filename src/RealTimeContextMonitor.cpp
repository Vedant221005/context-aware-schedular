#include "../include/RealTimeContextMonitor.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

RealTimeContextMonitor::RealTimeContextMonitor(
    std::chrono::milliseconds refreshInterval_)
    : refreshInterval(std::max(
          std::chrono::milliseconds(2000),
          std::min(std::chrono::milliseconds(5000), refreshInterval_))) {}

Context RealTimeContextMonitor::sample() {
    latest = collectContext();
    samples.push_back(latest);
    return latest;
}

const Context& RealTimeContextMonitor::getLatest() const {
    return latest;
}

const std::vector<Context>& RealTimeContextMonitor::getSamples() const {
    return samples;
}

std::chrono::milliseconds RealTimeContextMonitor::getRefreshInterval() const {
    return refreshInterval;
}

Context RealTimeContextMonitor::collectContext() {
    Context context;
    context.batteryLevel = readBatteryLevel();
    context.cpuTemperature = readCPUTemperature();
    context.cpuUtilization = readCPUUtilization();
    context.userActive = readUserActivity();
    context.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return context;
}

double RealTimeContextMonitor::readBatteryLevel() const {
#ifdef _WIN32
    SYSTEM_POWER_STATUS status{};
    if (GetSystemPowerStatus(&status) != 0
        && status.BatteryLifePercent != 255) {
        return static_cast<double>(status.BatteryLifePercent);
    }
#endif
    return 100.0;
}

double RealTimeContextMonitor::readCPUUtilization() {
#ifdef _WIN32
    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime) != 0) {
        const auto toValue = [](const FILETIME& value) {
            return (static_cast<unsigned long long>(value.dwHighDateTime) << 32)
                | value.dwLowDateTime;
        };
        const unsigned long long idle = toValue(idleTime);
        const unsigned long long kernel = toValue(kernelTime);
        const unsigned long long user = toValue(userTime);
        if (hasCpuSample) {
            const unsigned long long idleDelta = idle - previousIdle;
            const unsigned long long totalDelta =
                (kernel - previousKernel) + (user - previousUser);
            previousIdle = idle;
            previousKernel = kernel;
            previousUser = user;
            if (totalDelta > 0) {
                return std::max(
                    0.0,
                    std::min(100.0, 100.0
                        * (1.0 - static_cast<double>(idleDelta)
                            / static_cast<double>(totalDelta))));
            }
        } else {
            hasCpuSample = true;
            previousIdle = idle;
            previousKernel = kernel;
            previousUser = user;
        }
    }
#endif
    return hasCpuSample ? latest.cpuUtilization : 0.0;
}

double RealTimeContextMonitor::readCPUTemperature() const {
    // Windows does not expose a portable CPU temperature API.
    return -1.0;
}

bool RealTimeContextMonitor::readUserActivity() const {
#ifdef _WIN32
    LASTINPUTINFO inputInfo{};
    inputInfo.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&inputInfo) != 0) {
        const unsigned long long idleMilliseconds =
            GetTickCount64() - inputInfo.dwTime;
        return idleMilliseconds < 5000;
    }
#endif
    return true;
}
