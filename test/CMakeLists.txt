# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

if(NOT ENABLE_TEST)
  return()
endif()

find_package(Clang REQUIRED)
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(harness)

build_test_program(
  NAME unittest
  SRCS testMain.cpp GitHarnessTest.cpp StringTest.cpp IoTest.cpp ExceptionTest.cpp
       GitRepositoryTest.cpp ResultTest.cpp MutantsTest.cpp
       LoggerTest.cpp UniformMutantGeneratorTest.cpp MutationResultsTest.cpp
       MutationFactoryTest.cpp EvaluatorTest.cpp CoverageInfoTest.cpp
       GitSourceTreeTest.cpp XmlReportTest.cpp HtmlReportTest.cpp ReportTest.cpp
       ContainerTest.cpp RandomMutantGeneratorTest.cpp
       WeightedMutantGeneratorTest.cpp SourceLineTest.cpp
       ConfigTest.cpp WorkspaceTest.cpp ConsoleTest.cpp SubprocessTest.cpp
       CliConfigParserTest.cpp StatusLineTest.cpp
       TimestamperTest.cpp AORTest.cpp BORTest.cpp LCRTest.cpp RORTest.cpp SDLTest.cpp SORTest.cpp UOITest.cpp
       DocGeneratorTest.cpp StageTest.cpp ConfigValidatorTest.cpp BaselineTestStageTest.cpp
       EvaluationStageTest.cpp SignalHandlerTest.cpp YamlConfigParserTest.cpp YamlConfigWriterTest.cpp
  PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/test/include ${CMAKE_SOURCE_DIR}/test/harness
  LIBS sentinel-core git-harness ${LLVM_CLANG_LIBS} ${CMAKE_DL_LIBS}
)
