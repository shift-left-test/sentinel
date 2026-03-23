cmake_minimum_required(VERSION 3.13)
project(SentinelSample CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_library(stack src/Stack.cpp)
target_include_directories(stack PUBLIC src)

add_subdirectory(test)
