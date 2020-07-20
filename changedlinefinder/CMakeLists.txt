cmake_minimum_required(VERSION 3.5.1)
project(changed-line-finder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${GIT2_INCLUDE_DIR})

# Two ways to define variable: set(...) and file(GLOB ...)
# file(GLOB ...) allows wildcard additions
# set(SOURCES src/main.cpp)
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(changedlinefinder ${SOURCES})
target_link_libraries(changedlinefinder git2)

install(TARGETS changedlinefinder RUNTIME DESTINATION bin)
