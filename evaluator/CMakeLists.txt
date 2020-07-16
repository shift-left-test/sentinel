cmake_minimum_required(VERSION 3.5.1)
project(mutant-test-result-evaluator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(lib)

# Two ways to define variable: set(...) and file(GLOB ...)
# file(GLOB ...) allows wildcard additions
# set(SOURCES src/main.cpp)
# file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(evaluator src/main.cpp)
target_link_libraries (evaluator LINK_PUBLIC tinyxml)
install(TARGETS evaluator RUNTIME DESTINATION bin)
