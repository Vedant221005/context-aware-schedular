#pragma once

#include <vector>

#include "Process.h"
#include "SchedulerResult.h"

SchedulerResult runPriorityScheduling(const std::vector<Process>& processes);
