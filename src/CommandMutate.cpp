/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/GitRepository.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceTree.hpp"
#include "sentinel/CommandMutate.hpp"

namespace sentinel {
const char * cCommandMutateLoggerName = "CommandMutate";

CommandMutate::CommandMutate(args::Subparser& parser) :
    Command(parser), mMutantStr(parser, "MUTANT", "Mutant string", {'m', "mutant"}, args::Options::Required) {
}

int CommandMutate::run() {
  namespace fs = std::experimental::filesystem;
  fs::create_directories(mWorkDir.Get());
  fs::path workDir = fs::canonical(mWorkDir.Get());
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());

  auto logger = Logger::getLogger(cCommandMutateLoggerName);
  sentinel::Mutant m;
  std::istringstream iss(mMutantStr.Get());
  iss >> m;

  if (mIsVerbose.Get()) {
    logger->info(fmt::format("mutant: {}", mMutantStr.Get()));
    logger->info(fmt::format("backup dir: {}", workDir.string()));
  }

  sentinel::GitRepository repository(sourceRoot);
  repository.getSourceTree()->modify(m, workDir);

  return 0;
}

}  // namespace sentinel
