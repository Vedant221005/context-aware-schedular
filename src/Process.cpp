#include "../include/Process.h"
#include <iomanip>

Process::Process()
    : pid(0), arrivalTime(0), burstTime(0), priority(0), cpuUsage(0.0),
      batteryImpact(0.0), temperatureImpact(0.0), foregroundTask(false),
      backgroundTask(false), contextScore(0.0) {}

Process::Process(int pid_, int arrival, int burst, int priority_)
    : pid(pid_), arrivalTime(arrival), burstTime(burst), priority(priority_),
      cpuUsage(0.0), batteryImpact(0.0), temperatureImpact(0.0),
      foregroundTask(false), backgroundTask(true), contextScore(0.0) {}

int Process::getPid() const { return pid; }
void Process::setPid(int p) { pid = p; }

int Process::getArrivalTime() const { return arrivalTime; }
void Process::setArrivalTime(int t) { arrivalTime = t; }

int Process::getBurstTime() const { return burstTime; }
void Process::setBurstTime(int b) { burstTime = b; }

int Process::getPriority() const { return priority; }
void Process::setPriority(int p) { priority = p; }

double Process::getContextScore() const { return contextScore; }
void Process::setContextScore(double s) { contextScore = s; }

void Process::displayProcess() const {
    std::cout << "Process(pid=" << pid << ", arrival=" << arrivalTime
              << ", burst=" << burstTime << ", priority=" << priority
              << ")\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  cpuUsage=" << cpuUsage << ", batteryImpact=" << batteryImpact
              << ", temperatureImpact=" << temperatureImpact
              << ", contextScore=" << contextScore << "\n";
}
