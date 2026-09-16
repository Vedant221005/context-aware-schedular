#include "../include/ThermalModel.h"

#include <algorithm>

ThermalModel::Simulation ThermalModel::simulate(
    double executionTime,
    int contextSwitches,
    double schedulerFactor) {
    const int steps = std::max(0, static_cast<int>(executionTime));
    const double factor = std::max(0.5, schedulerFactor);
    const double switchHeat = steps > 0
        ? static_cast<double>(std::max(0, contextSwitches)) * 0.02 / steps
        : 0.0;

    Simulation result;
    double temperature = 35.0;
    double temperatureSum = 0.0;
    bool wasThermal = false;
    for (int step = 0; step < steps; ++step) {
        const double heat = 0.12 * factor + switchHeat;
        temperature = updateTemperature(temperature, heat);
        temperatureSum += temperature;
        result.peakTemperature = std::max(result.peakTemperature, temperature);
        const bool thermal = isThermalEvent(temperature);
        if (thermal && !wasThermal) {
            ++result.thermalEvents;
        }
        wasThermal = thermal;
    }
    result.averageTemperature = steps > 0
        ? temperatureSum / static_cast<double>(steps) : 35.0;
    return result;
}

double ThermalModel::updateTemperature(
    double currentTemperature,
    double power) {
    const double heating = std::max(0.0, power);
    return std::clamp(currentTemperature + heating - 0.08, 35.0, 100.0);
}

bool ThermalModel::isThermalEvent(double temperature) {
    return temperature >= 80.0;
}
