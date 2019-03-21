cmake_minimum_required(VERSION 3.12)
project(rooks)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("CMake build type = ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "-Wpedantic -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)

add_executable(rooks main.cpp)
link_libraries(rooks)
