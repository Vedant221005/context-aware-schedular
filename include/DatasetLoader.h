#pragma once

#include <string>
#include <vector>

#include "Process.h"

class DatasetLoader {
public:
    std::vector<Process> loadCSV(const std::string& filename);
};
