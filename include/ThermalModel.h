#pragma once

class ThermalModel {
public:
    struct Simulation {
        double averageTemperature = 35.0;
        double peakTemperature = 35.0;
        int thermalEvents = 0;
    };

    static Simulation simulate(
        double executionTime,
        int contextSwitches,
        double schedulerFactor);

    static double updateTemperature(
        double currentTemperature,
        double power);

    static bool isThermalEvent(double temperature);
};
