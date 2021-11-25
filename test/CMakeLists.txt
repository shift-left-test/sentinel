# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

if(NOT ENABLE_TEST)
  return()
endif()

find_package(Clang REQUIRED)
link_directories(${LLVM_LIBRARY_DIRS})

build_test_program(
  NAME unittest
  SRCS testMain.cpp GitHarnessTest.cpp StringTest.cpp ExceptionTest.cpp
       GitRepositoryTest.cpp ResultTest.cpp MutantsTest.cpp
       LoggerTest.cpp UniformMutantGeneratorTest.cpp MutationResultsTest.cpp
       MutationFactoryTest.cpp EvaluatorTest.cpp CoverageInfoTest.cpp
       GitSourceTreeTest.cpp XMLReportTest.cpp HTMLReportTest.cpp ReportTest.cpp
       ContainerTest.cpp RandomMutantGeneratorTest.cpp
       WeightedMutantGeneratorTest.cpp MainCLITest.cpp
  PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/test/include
  LIBS sentinel-core git-harness ${LLVM_CLANG_LIBS}
)
