cmake_minimum_required(VERSION 3.13)
project(SentinelSample CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_library(stack src/Stack.cpp)
target_include_directories(stack PUBLIC src)

add_library(tokenizer src/Tokenizer.cpp)
target_include_directories(tokenizer PUBLIC src)

add_library(calculator src/Calculator.cpp)
target_include_directories(calculator PUBLIC src)
target_link_libraries(calculator PUBLIC stack tokenizer)

add_executable(calc src/main.cpp)
target_link_libraries(calc calculator)

add_subdirectory(test)
