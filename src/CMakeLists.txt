# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

find_package(LibGit2 REQUIRED)
find_package(Clang REQUIRED)
link_directories(${LLVM_LIBRARY_DIRS})

add_library(sentinel-core STATIC
  SourceLine.cpp SourceTree.cpp Mutant.cpp Repository.cpp
  MutantGenerator.cpp UniformMutantGenerator.cpp RandomMutantGenerator.cpp WeightedMutantGenerator.cpp
  MutationFactory.cpp GitRepository.cpp Result.cpp XmlParser.cpp GoogleTestXmlParser.cpp CTestXmlParser.cpp QTestXmlParser.cpp Evaluator.cpp
  MutationResult.cpp GitSourceTree.cpp Timestamper.cpp
  MutationSummary.cpp Report.cpp XmlReport.cpp HtmlReport.cpp
  Logger.cpp operators/aor.cpp operators/bor.cpp operators/lcr.cpp
  operators/ror.cpp operators/sdl.cpp operators/sor.cpp operators/uoi.cpp
  operators/MutationOperator.cpp Subprocess.cpp
  docGenerator/DocGenerator.cpp docGenerator/IndexHtmlGenerator.cpp docGenerator/SrcHtmlGenerator.cpp
  CliConfigParser.cpp YamlConfigParser.cpp YamlConfigWriter.cpp Config.cpp ConfigValidator.cpp CoverageInfo.cpp Workspace.cpp StatusLine.cpp Stage.cpp SignalHandler.cpp
  stages/OriginalBuildStage.cpp stages/OriginalTestStage.cpp
  stages/GenerationStage.cpp stages/DryRunStage.cpp stages/EvaluationStage.cpp stages/ReportStage.cpp
)
target_include_directories(sentinel-core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(sentinel-core SYSTEM PUBLIC ${CLANG_INCLUDE_DIRS})
target_link_libraries(sentinel-core PUBLIC
  tinyxml2 fmt yaml-cpp ${LIBGIT2_LIBRARIES} stdc++fs args
  $<$<BOOL:${CMAKE_TESTING_ENABLED}>:coverage_options>
)

add_executable(sentinel main.cpp)
target_link_libraries(sentinel PRIVATE sentinel-core ${LLVM_CLANG_LIBS} ${CMAKE_DL_LIBS})
install(TARGETS sentinel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
