/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/io.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Evaluator::Evaluator(const std::filesystem::path& expectedResultDir, const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath),
    mExpectedResult(expectedResultDir.string()) {
  if (mExpectedResult.checkPassedTCEmpty()) {
    throw InvalidArgumentException(fmt::format("No passed TC in Expected Result({0})", expectedResultDir.string()));
  }
}

MutationResult Evaluator::compare(const Mutant& mut, const std::filesystem::path& actualResultDir,
                                  TestExecutionState testState) {
  std::string killingTC;
  std::string errorTC;
  MutationState state;

  switch (testState) {
    case TestExecutionState::BUILD_FAILURE:
      state = MutationState::BUILD_FAILURE;
      break;
    case TestExecutionState::TIMEOUT:
      state = MutationState::TIMEOUT;
      break;
    case TestExecutionState::UNCOVERED:
      state = MutationState::SURVIVED;
      break;
    case TestExecutionState::RUNTIME_ERROR:
      state = MutationState::RUNTIME_ERROR;
      break;
    case TestExecutionState::SUCCESS: {
      Result actualResult(actualResultDir.string());
      state = Result::compare(mExpectedResult, actualResult, &killingTC, &errorTC);
      break;
    }
  }

  return {mut, killingTC, errorTC, state};
}

MutationResult Evaluator::compareAndSaveMutationResult(const Mutant& mut, const std::filesystem::path& actualResultDir,
                                                       const std::filesystem::path& evalFilePath,
                                                       TestExecutionState testState) {
  auto outDir = evalFilePath.parent_path();
  io::ensureDirectoryExists(outDir);

  MutationResult ret = compare(mut, actualResultDir, testState);

  std::ofstream outFile(evalFilePath.string(), std::ios::out | std::ios::app);
  outFile << ret << "\n";
  outFile.close();

  return ret;
}

}  // namespace sentinel
