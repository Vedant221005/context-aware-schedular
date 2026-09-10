#include "../include/Metrics.h"
#include <iomanip>

Metrics::Metrics()
    : waitingTime(0.0), turnaroundTime(0.0), responseTime(0.0), cpuUtilization(0.0),
      throughput(0.0), contextSwitches(0), powerConsumption(0.0), thermalEfficiency(0.0) {}

void Metrics::displayMetrics() const {
    std::cout << "Metrics:\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  waitingTime=" << waitingTime << ", turnaroundTime=" << turnaroundTime
              << ", responseTime=" << responseTime << "\n";
    std::cout << "  cpuUtil=" << cpuUtilization << "%, throughput=" << throughput
              << ", contextSwitches=" << contextSwitches << "\n";
    std::cout << "  powerConsumption=" << powerConsumption << ", thermalEfficiency="
              << thermalEfficiency << "\n";
}
