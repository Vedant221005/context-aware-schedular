#include "../include/ContextTraceLoader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<ContextSnapshot> ContextTraceLoader::loadCSV(
    const std::string& filename) const {
    std::vector<ContextSnapshot> snapshots;
    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Unable to open context trace: " << filename << "\n";
        return snapshots;
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
            if (line.find("Time") != std::string::npos) {
                continue;
            }
        }

        std::stringstream stream(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(stream, field, ',')) {
            fields.push_back(field);
        }
        if (fields.size() != 5) {
            std::cerr << "Skipping invalid context row " << lineNumber
                      << " in " << filename << "\n";
            continue;
        }

        try {
            snapshots.push_back({
                std::stoi(fields[0]),
                std::stod(fields[1]),
                std::stod(fields[2]),
                std::stod(fields[3]),
                std::stoi(fields[4]) != 0
            });
        } catch (const std::invalid_argument&) {
            std::cerr << "Skipping invalid context row " << lineNumber
                      << " in " << filename << "\n";
        } catch (const std::out_of_range&) {
            std::cerr << "Skipping invalid context row " << lineNumber
                      << " in " << filename << "\n";
        }
    }

    std::sort(snapshots.begin(), snapshots.end(),
        [](const ContextSnapshot& left, const ContextSnapshot& right) {
            return left.time < right.time;
        });
    return snapshots;
}
