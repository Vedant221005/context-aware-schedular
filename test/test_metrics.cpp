#include "../include/Metrics.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Metrics default constructor initializes zeros") {
    Metrics m;
    REQUIRE(m.waitingTime == Approx(0.0));
    REQUIRE(m.turnaroundTime == Approx(0.0));
    REQUIRE(m.responseTime == Approx(0.0));
    REQUIRE(m.cpuUtilization == Approx(0.0));
    REQUIRE(m.throughput == Approx(0.0));
    REQUIRE(m.contextSwitches == 0);
    REQUIRE(m.powerConsumption == Approx(0.0));
    REQUIRE(m.thermalEfficiency == Approx(0.0));
}

TEST_CASE("Metrics fields can be assigned and read") {
    Metrics m;
    m.waitingTime = 5.5;
    m.contextSwitches = 3;
    REQUIRE(m.waitingTime == Approx(5.5));
    REQUIRE(m.contextSwitches == 3);
}
