#pragma once

#include <vector>

#include "Process.h"
#include "SchedulerResult.h"

SchedulerResult runRoundRobin(const std::vector<Process>& processes, int quantum = 4);
