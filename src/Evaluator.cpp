/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Evaluator::Evaluator(const std::filesystem::path& expectedResultDir) :
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

  MutationResult result(mut, killingTC, errorTC, state);
  if (testState == TestExecutionState::UNCOVERED) {
    result.setUncovered(true);
  }
  return result;
}

}  // namespace sentinel
