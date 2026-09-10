#include "../include/ContextManager.h"
#include <iomanip>

ContextManager::ContextManager()
    : batteryLevel(100.0), cpuTemperature(35.0), cpuUtilization(0.0), userActive(true) {}

void ContextManager::updateBatteryLevel(double simulatedValue) {
    batteryLevel = simulatedValue;
}

void ContextManager::updateTemperature(double simulatedValue) {
    cpuTemperature = simulatedValue;
}

void ContextManager::updateCPUUtilization(double simulatedValue) {
    cpuUtilization = simulatedValue;
}

void ContextManager::updateUserActivity(bool active) { userActive = active; }

void ContextManager::displayContext() const {
    std::cout << "Context:\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  batteryLevel=%" << batteryLevel << ", cpuTemp=" << cpuTemperature
              << " C, cpuUtil=" << cpuUtilization << "%, userActive="
              << (userActive ? "true" : "false") << "\n";
}
