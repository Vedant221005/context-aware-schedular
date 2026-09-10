#include "../include/DatasetLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<Process> DatasetLoader::loadCSV(const std::string& filename) {
    std::vector<Process> processes;
    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Unable to open dataset: " << filename << "\n";
        return processes;
    }

    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }
        if (firstLine) {
            firstLine = false;
            if (line.find("PID") != std::string::npos) {
                continue;
            }
        }

        std::stringstream stream(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(stream, field, ',')) {
            fields.push_back(field);
        }
        if (fields.size() != 8) {
            std::cerr << "Skipping invalid row " << lineNumber
                      << " in " << filename << "\n";
            continue;
        }

        try {
            Process process(
                std::stoi(fields[0]),
                std::stoi(fields[1]),
                std::stoi(fields[2]),
                std::stoi(fields[3]));
            process.cpuUsage = std::stod(fields[4]);
            process.batteryImpact = std::stod(fields[5]);
            process.temperatureImpact = std::stod(fields[6]);
            process.foregroundTask = std::stoi(fields[7]) != 0;
            process.backgroundTask = !process.foregroundTask;
            processes.push_back(process);
        } catch (const std::invalid_argument&) {
            std::cerr << "Skipping invalid row " << lineNumber
                      << " in " << filename << "\n";
        } catch (const std::out_of_range&) {
            std::cerr << "Skipping invalid row " << lineNumber
                      << " in " << filename << "\n";
        }
    }

    return processes;
}
