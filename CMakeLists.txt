#cmake_minimum_required(VERSION 3.10)
project(lge-mutation-testing VERSION 0.1)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(include)
include_directories(include/mutation-operators)
set(LLVM_LINK_COMPONENTS support)

# Two ways to define variable: set(...) and file(GLOB ...)
# file(GLOB ...) allows wildcard additions
# set(SOURCES src/main.cpp)
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_clang_executable(lemon ${SOURCES})

target_link_libraries(lemon
  PRIVATE
  clangTooling
  clangAST
  clangBasic
  clangASTMatchers
)

install(TARGETS lemon RUNTIME DESTINATION bin)
