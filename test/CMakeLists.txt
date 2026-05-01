# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

if(NOT CMAKE_TESTING_ENABLED)
  return()
endif()

find_package(Clang REQUIRED)
find_package(GTest REQUIRED)
include(GoogleTest)

link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(harness)

add_executable(unittest
  testMain.cpp GitHarnessTest.cpp StringTest.cpp IoTest.cpp ExceptionTest.cpp
  GitRepositoryTest.cpp ResultTest.cpp MutantsTest.cpp
  LoggerTest.cpp UniformMutantGeneratorTest.cpp MutationResultsTest.cpp
  MutationFactoryTest.cpp EvaluatorTest.cpp CoverageInfoTest.cpp
  GitSourceTreeTest.cpp XmlReportTest.cpp HtmlReportTest.cpp ReportTest.cpp
  RandomMutantGeneratorTest.cpp
  WeightedMutantGeneratorTest.cpp SourceLineTest.cpp
  ConfigTest.cpp WorkspaceTest.cpp ConsoleTest.cpp SubprocessTest.cpp
  CliConfigParserTest.cpp StatusLineTest.cpp
  TimestamperTest.cpp AORTest.cpp BORTest.cpp LCRTest.cpp RORTest.cpp SDLTest.cpp SORTest.cpp UOITest.cpp
  StageTest.cpp ConfigValidatorTest.cpp
  EvaluationStageTest.cpp DryRunStageTest.cpp ReportStageTest.cpp
  OriginalBuildStageTest.cpp OriginalTestStageTest.cpp
  SignalHandlerTest.cpp OomHandlerTest.cpp YamlConfigWriterTest.cpp
  FormatterTest.cpp
  Utf8CharTest.cpp
  PartitionedWorkspaceMergerTest.cpp
)
target_include_directories(unittest PRIVATE
  ${CMAKE_SOURCE_DIR}/test/include
  ${CMAKE_SOURCE_DIR}/test/harness
)
target_link_libraries(unittest PRIVATE
  sentinel-core git-harness ${LLVM_CLANG_LIBS} ${CMAKE_DL_LIBS} coverage_options
  GTest::gmock GTest::gmock_main
)
gtest_discover_tests(unittest)
