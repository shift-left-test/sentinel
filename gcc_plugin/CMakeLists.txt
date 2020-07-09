#cmake_minimum_required (VERSION 3.10.2)

project(gcc_mutant_finder_plugin)

set(CMAKE_CXX_FLAGS "-fpic")
execute_process(
    COMMAND gcc -print-file-name=plugin
    OUTPUT_VARIABLE GCC_PLUGIN_DEV_PATH
)
string(STRIP ${GCC_PLUGIN_DEV_PATH} GCC_PLUGIN_DEV_PATH)
include_directories( "${GCC_PLUGIN_DEV_PATH}/include")

add_library(gcc_mutant_finder_plugin SHARED
    src/gcc_mutant_finder.cpp
)

enable_testing()

add_test(
    NAME ast_dump
    COMMAND gcc -fplugin=${CMAKE_BINARY_DIR}/libgcc_mutant_finder_plugin.so ${CMAKE_SOURCE_DIR}/test/sample.cpp -c
)