#pragma once

#include <string>
#include <vector>

struct ContextSnapshot {
    int time = 0;
    double battery = 0.0;
    double temperature = 0.0;
    double cpuUtilization = 0.0;
    bool userActive = false;
};

class ContextTraceLoader {
public:
    std::vector<ContextSnapshot> loadCSV(const std::string& filename) const;
};
