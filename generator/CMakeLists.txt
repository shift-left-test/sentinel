cmake_minimum_required(VERSION 3.5.1)
project(mutant-generator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Two ways to define variable: set(...) and file(GLOB ...)
# file(GLOB ...) allows wildcard additions
# set(SOURCES src/main.cpp)
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(generator ${SOURCES})
install(TARGETS generator RUNTIME DESTINATION bin)
