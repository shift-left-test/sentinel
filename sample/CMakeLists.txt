cmake_minimum_required(VERSION 3.13)
project(SentinelSample CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_library(coverage_options INTERFACE)
target_compile_options(coverage_options INTERFACE -O0 -fprofile-arcs -ftest-coverage)
target_link_options(coverage_options INTERFACE -fprofile-arcs -ftest-coverage)

add_subdirectory(src)
add_subdirectory(test)
