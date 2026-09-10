#include "../include/ContextScoreEngine.h"

#include <iomanip>
#include <iostream>

ContextScoreEngine::ScoreBreakdown ContextScoreEngine::calculateWeights(
    const Process& process,
    const ContextManager& context) const {
    const double foregroundWeight = process.foregroundTask ? 100.0 : 40.0;

    double batteryWeight = 20.0;
    if (context.batteryLevel > 80.0) {
        batteryWeight = 100.0;
    } else if (context.batteryLevel >= 50.0) {
        batteryWeight = 80.0;
    } else if (context.batteryLevel >= 20.0) {
        batteryWeight = 50.0;
    }

    double temperatureWeight = 30.0;
    if (context.cpuTemperature < 60.0) {
        temperatureWeight = 100.0;
    } else if (context.cpuTemperature <= 75.0) {
        temperatureWeight = 70.0;
    }

    double cpuUtilizationWeight = 40.0;
    if (context.cpuUtilization < 50.0) {
        cpuUtilizationWeight = 100.0;
    } else if (context.cpuUtilization <= 80.0) {
        cpuUtilizationWeight = 70.0;
    }

    const double userActivityWeight = context.userActive ? 100.0 : 50.0;

    return {
        foregroundWeight,
        batteryWeight,
        temperatureWeight,
        cpuUtilizationWeight,
        userActivityWeight
    };
}

double ContextScoreEngine::calculateContextScore(
    const Process& process,
    const ContextManager& context) {
    const ScoreBreakdown weights = calculateWeights(process, context);
    return (0.30 * weights.foregroundWeight)
        + (0.20 * weights.batteryWeight)
        + (0.20 * weights.temperatureWeight)
        + (0.15 * weights.cpuUtilizationWeight)
        + (0.15 * weights.userActivityWeight);
}

void ContextScoreEngine::displayContextScoreBreakdown(
    const Process& process,
    const ContextManager& context) {
    const ScoreBreakdown weights = calculateWeights(process, context);
    const double score = calculateContextScore(process, context);

    std::cout << "\nProcess ID: " << process.pid << "\n\n";
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "Foreground Weight      : " << weights.foregroundWeight << "\n";
    std::cout << "Battery Weight         : " << weights.batteryWeight << "\n";
    std::cout << "Temperature Weight     : " << weights.temperatureWeight << "\n";
    std::cout << "CPU Utilization Weight : " << weights.cpuUtilizationWeight << "\n";
    std::cout << "User Activity Weight   : " << weights.userActivityWeight << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nFinal Context Score    : " << score << "\n";
}
