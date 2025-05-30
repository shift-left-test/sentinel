/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/Evaluator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/CommandEvaluate.hpp"


namespace sentinel {
const char * cCommandEvaluateLoggerName = "CommandEvaluate";

CommandEvaluate::CommandEvaluate(args::Subparser& parser) : Command(parser),
  mMutantStr(parser, "mutant",
    "Mutant string",
    {'m', "mutant"}, args::Options::Required),
  mExpectedDir(parser, "PATH",
    "Expected result directory",
    {'e', "expected"}, args::Options::Required),
  mActualDir(parser, "PATH",
    "Actual result directory",
    {'a', "actual"}, args::Options::Required),
  mEvalFile(parser, "FILENAME",
    "Evaluated output file name which will be joined with output-dir.",
    {"evaluation-file"}, "EvaluationResults"),
  mTestState(parser, "TEST_STATE",
    R"asdf(Select the state of the test to be evaluated, one of ['success', 'build_failure', 'timeout', 'uncovered'].)asdf",
    {"test-state"}, "success") {
}

int CommandEvaluate::run() {
  namespace fs = std::experimental::filesystem;

  if (mTestState.Get() != "build_failure" && mTestState.Get() != "timeout"
      && mTestState.Get() != "success" && mTestState.Get() != "uncovered") {
      throw InvalidArgumentException(fmt::format(
          "Invalid value for test-state option: {0}", mTestState.Get()));
  }

  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  std::string outputDirStr;
  if (mOutputDir.Get().empty()) {
    outputDirStr = ".";
  } else {
    outputDirStr = mOutputDir.Get();
  }
  fs::create_directories(outputDirStr);
  fs::path outputDir = fs::canonical(outputDirStr);

  auto logger = Logger::getLogger(cCommandEvaluateLoggerName);

  sentinel::Mutant m;
  std::istringstream iss(mMutantStr.Get());
  iss >> m;

  if (mIsVerbose.Get()) {
    logger->info(fmt::format("mutant: {}", mMutantStr));
  }

  sentinel::Evaluator evaluator(mExpectedDir.Get(), sourceRoot);

  evaluator.compareAndSaveMutationResult(m, mActualDir.Get(),
    outputDir / mEvalFile.Get(), mTestState.Get());

  return 0;
}
}  // namespace sentinel
