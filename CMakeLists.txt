cmake_minimum_required(VERSION 3.5.1)
project(lge-mutation-testing)

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(include)
include_directories(include/mutation-operators)

if (NOT LLVM_CONFIG_BIN)
  set(LLVM_CONFIG_BIN "llvm-config")
endif()

message(STATUS ${LLVM_CONFIG_BIN})

execute_process(
  COMMAND ${LLVM_CONFIG_BIN} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")

execute_process(
  COMMAND ${LLVM_CONFIG_BIN} --ldflags
  OUTPUT_VARIABLE LLVM_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")

execute_process(
  COMMAND ${LLVM_CONFIG_BIN} --libs
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (CLANG_INCLUDE_PATH)
  include_directories (${CLANG_INCLUDE_PATH} )
  MESSAGE(STATUS ${CLANG_INCLUDE_PATH})
endif()

# Two ways to define variable: set(...) and file(GLOB ...)
# file(GLOB ...) allows wildcard additions
# set(SOURCES src/main.cpp)
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(lemon ${SOURCES})
message(STATUS ${LLVM_CXXFLAGS})
message(STATUS ${LLVM_LDFLAGS})
message(STATUS ${LLVM_LIBS})
target_link_libraries(lemon
  PRIVATE
  clangFrontend
  clangParse
  clangRewrite
  clangRewriteFrontend
  clangSerialization
  clangSema
  clangEdit
  clangLex
  clangAnalysis
  clangAST
  clangASTMatchers
  clangStaticAnalyzerFrontend
  clangTooling
  clangDriver
  clangBasic
  ${LLVM_LIBS}
  pthread
  z
)

install(TARGETS lemon RUNTIME DESTINATION bin)
