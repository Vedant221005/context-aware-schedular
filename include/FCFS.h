#pragma once

#include <vector>

#include "Process.h"
#include "SchedulerResult.h"

SchedulerResult runFCFS(const std::vector<Process>& processes);
