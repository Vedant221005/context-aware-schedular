#include "../include/Process.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Process default constructor sets zeros and defaults") {
    Process p;
    REQUIRE(p.getPid() == 0);
    REQUIRE(p.getArrivalTime() == 0);
    REQUIRE(p.getBurstTime() == 0);
    REQUIRE(p.getPriority() == 0);
    REQUIRE(p.getContextScore() == Approx(0.0));
}

TEST_CASE("Process parameterized constructor sets given values") {
    Process p(42, 10, 20, 7);
    REQUIRE(p.getPid() == 42);
    REQUIRE(p.getArrivalTime() == 10);
    REQUIRE(p.getBurstTime() == 20);
    REQUIRE(p.getPriority() == 7);
}
