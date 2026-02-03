# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

find_package(LibGit2 REQUIRED)
find_package(Clang REQUIRED)
find_package(Curses REQUIRED)
find_library(FORM_LIBRARY form)
find_library(PANEL_LIBRARY panel)
link_directories(${LLVM_LIBRARY_DIRS})

build_static_library(
  NAME sentinel-core
  SRCS SourceLine.cpp SourceTree.cpp Mutant.cpp
       UniformMutantGenerator.cpp RandomMutantGenerator.cpp
       WeightedMutantGenerator.cpp
       MutationFactory.cpp GitRepository.cpp Result.cpp XmlParser.cpp GoogleTestXmlParser.cpp QTestXmlParser.cpp Evaluator.cpp
       MutationResult.cpp GitSourceTree.cpp
       Report.cpp XMLReport.cpp HTMLReport.cpp
       Logger.cpp operators/aor.cpp operators/bor.cpp operators/lcr.cpp
       operators/ror.cpp operators/sdl.cpp operators/sor.cpp operators/uoi.cpp
       operators/MutationOperator.cpp util/Subprocess.cpp
       docGenerator/IndexHTMLGenerator.cpp docGenerator/SrcHTMLGenerator.cpp
       Command.cpp CommandPopulate.cpp CommandMutate.cpp CommandEvaluate.cpp
       CommandReport.cpp CommandRun.cpp MainCLI.cpp ncstream/NcWindowStreambuf.cpp
       CommandGui.cpp CoverageInfo.cpp
  PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include
  PUBLIC_SYSTEM_HEADERS ${CLANG_INCLUDE_DIRS} ${CURSES_INCLUDE_DIR}
  LIBS tinyxml2 fmt ${LIBGIT2_LIBRARIES} stdc++fs args ${FORM_LIBRARY} ${PANEL_LIBRARY} ${CURSES_LIBRARIES}
  NO_INSTALL
)

build_program(
  NAME sentinel
  SRCS main.cpp
  LIBS sentinel-core ${LLVM_CLANG_LIBS} ${CMAKE_DL_LIBS}
)
