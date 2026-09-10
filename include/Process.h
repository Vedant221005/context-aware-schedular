#pragma once

#include <string>
#include <iostream>

class Process {
public:
    // Attributes
    int pid;
    int arrivalTime;
    int burstTime;
    int priority;
    double cpuUsage;
    double batteryImpact;
    double temperatureImpact;
    bool foregroundTask;
    bool backgroundTask;
    double contextScore;
    int deadline;
    int period;
    bool isRealTime;

    // Constructors
    Process();
    Process(int pid, int arrival, int burst, int priority);

    // Getters / Setters (simple public fields for now; helper methods below)
    int getPid() const;
    void setPid(int p);

    int getArrivalTime() const;
    void setArrivalTime(int t);

    int getBurstTime() const;
    void setBurstTime(int b);

    int getPriority() const;
    void setPriority(int p);

    double getContextScore() const;
    void setContextScore(double s);

    // Utility
    void displayProcess() const;
};
