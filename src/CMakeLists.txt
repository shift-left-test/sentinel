# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

find_package(LibGit2 REQUIRED)
find_package(Clang REQUIRED)
link_directories(${LLVM_LIBRARY_DIRS})

build_static_library(
  NAME sentinel-core
  SRCS SourceLine.cpp SourceTree.cpp Mutant.cpp
       MutantGenerator.cpp UniformMutantGenerator.cpp RandomMutantGenerator.cpp WeightedMutantGenerator.cpp
       MutationFactory.cpp GitRepository.cpp Result.cpp XmlParser.cpp GoogleTestXmlParser.cpp CTestXmlParser.cpp QTestXmlParser.cpp Evaluator.cpp
       MutationResult.cpp GitSourceTree.cpp TimeStamper.cpp
       Report.cpp XMLReport.cpp HTMLReport.cpp
       Logger.cpp operators/aor.cpp operators/bor.cpp operators/lcr.cpp
       operators/ror.cpp operators/sdl.cpp operators/sor.cpp operators/uoi.cpp
       operators/MutationOperator.cpp Subprocess.cpp
       docGenerator/IndexHTMLGenerator.cpp docGenerator/SrcHTMLGenerator.cpp
       Command.cpp CommandRun.cpp SentinelConfig.cpp MainCLI.cpp CoverageInfo.cpp Workspace.cpp StatusLine.cpp
  PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include
  PUBLIC_SYSTEM_HEADERS ${CLANG_INCLUDE_DIRS}
  LIBS tinyxml2 fmt yaml-cpp ${LIBGIT2_LIBRARIES} stdc++fs args
  NO_INSTALL
)

build_program(
  NAME sentinel
  SRCS main.cpp
  LIBS sentinel-core ${LLVM_CLANG_LIBS} ${CMAKE_DL_LIBS}
)
