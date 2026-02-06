/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <experimental/filesystem>
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include "sentinel/CommandPopulate.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"

namespace sentinel {

const char * cCommandPopulateLoggerName = "CommandPopulate";

CommandPopulate::CommandPopulate(args::Subparser& parser) : Command(parser),
  mBuildDir(parser, "PATH",
    "Build command output directory",
    {'b', "build-dir"}, "."),
  mCompileDbDir(parser, "PATH",
    "Directory containing compile_commands.json file",
    {"compiledb"}),
  mScope(parser, "SCOPE",
    "Diff scope, one of ['commit', 'all'].",
    {'s', "scope"}, "all"),
  mExtensions(parser, "EXTENSION",
    "Extentions of source file which could be mutated.",
    {'t', "extension"}, {"cxx", "cpp", "cc", "c", "c++", "cu"}),
  mPatterns(parser, "PATTERN",
    "Path or pattern",
    {'p', "pattern"}),
  mExcludes(parser, "PAT",
    "Exclude paths matching fnmatch-style patterns",
    {'e', "exclude"}),
  mLimit(parser, "COUNT",
    "Maximum generated mutable count.",
    {'l', "limit"}, 10),
  mMutableFilename(parser, "PATH",
    "Populated result file name which will be created at output-dir.",
    {"mutants-file-name"}, "mutables.db"),
  mGenerator(parser, "GEN",
    "Select mutant generator type, one of ['uniform', 'random', 'weighted'].",
    {"generator"}, "uniform"),
  mSeed(parser, "SEED",
    "Select random seed.",
    {"seed"}, std::random_device {}()) {
}

int CommandPopulate::run() {
  namespace fs = std::experimental::filesystem;
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  std::string outputDirStr;
  if (mOutputDir.Get().empty()) {
    outputDirStr = ".";
  } else {
    outputDirStr = mOutputDir.Get();
  }
  fs::create_directories(outputDirStr);
  fs::path outputDir = fs::canonical(outputDirStr);

  auto logger = Logger::getLogger(cCommandPopulateLoggerName);
  if (mIsVerbose.Get()) {
    logger->info(fmt::format("source root:{}", sourceRoot.string()));
    for (auto& exclude : mExcludes.Get()) {
      logger->info(fmt::format("exclude:{}", exclude));
    }
    for (auto& extension : mExtensions.Get()) {
      logger->info(fmt::format("extension:{}", extension));
    }
    for (auto& pattern : mPatterns.Get()) {
      logger->info(fmt::format("pattern:{}", pattern));
    }
    logger->info(fmt::format("limit:{}", mLimit.Get()));
    logger->info(fmt::format("generator:{}", mGenerator.Get()));
    logger->info(fmt::format("random seed:{}", mSeed.Get()));
  }
  auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot,
                                                        mExtensions.Get(),
                                                        mPatterns.Get(),
                                                        mExcludes.Get());
  sentinel::SourceLines sourceLines = repo->getSourceLines(mScope.Get());

  // Shuffle target lines to reduce mutant selecting time.
  std::shuffle(std::begin(sourceLines), std::end(sourceLines), std::mt19937(mSeed.Get()));

  std::string compileDbDir = (mCompileDbDir.Get().empty()) ? mBuildDir.Get() : mCompileDbDir.Get();
  std::shared_ptr<MutantGenerator> generator = MutantGenerator::getInstance(mGenerator.Get(), compileDbDir);

  sentinel::MutationFactory mutationFactory(generator);

  sentinel::Mutants mutants = mutationFactory.populate(sourceRoot, sourceLines, mLimit.Get(), mSeed.Get());
  if (mIsVerbose) {
    for (auto& mutant : mutants) {
      logger->info(fmt::format("mutant: {}", mutant.str()));
    }
  }

  mutants.save(outputDir / mMutableFilename.Get());

  return 0;
}

}  // namespace sentinel
