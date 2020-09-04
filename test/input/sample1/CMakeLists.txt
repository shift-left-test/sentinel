# cmake_minimum_required(VERSION 3.10)

# set the project name
# project(sample1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add the executable
# add_executable(sample1 sample1.cpp)
add_library(sample1 STATIC sample1.cpp)
