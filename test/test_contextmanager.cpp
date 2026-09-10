#include "../include/ContextManager.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("ContextManager default values are sensible") {
    ContextManager c;
    REQUIRE(c.batteryLevel == Approx(100.0));
    REQUIRE(c.cpuTemperature == Approx(35.0));
    REQUIRE(c.cpuUtilization == Approx(0.0));
    REQUIRE(c.userActive == true);
}

TEST_CASE("ContextManager update methods change state") {
    ContextManager c;
    c.updateBatteryLevel(55.5);
    c.updateTemperature(60.1);
    c.updateCPUUtilization(12.3);
    c.updateUserActivity(false);

    REQUIRE(c.batteryLevel == Approx(55.5));
    REQUIRE(c.cpuTemperature == Approx(60.1));
    REQUIRE(c.cpuUtilization == Approx(12.3));
    REQUIRE(c.userActive == false);
}
