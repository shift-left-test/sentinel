# cmake_minimum_required(VERSION 3.10)

# set the project name
# project(sample1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add the executable
# add_executable(sample1 sample1.cpp)
add_library(sample1 STATIC sample1.cpp)

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_executable(sample1_test sample1.cpp test_main.cpp)
target_link_libraries(sample1_test gtest gtest_main pthread)

add_test(
    NAME sample1_test
    COMMAND sample1_test
)