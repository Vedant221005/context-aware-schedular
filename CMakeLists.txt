cmake_minimum_required(VERSION 3.10)
project(context_aware_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(context-aware-scheduler ${SOURCES})

# Project foundation and scheduling simulation entry point.
