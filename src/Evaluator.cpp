/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/io.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Evaluator::Evaluator(const std::filesystem::path& expectedResultDir, const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath),
    mCanonicalSourcePath(fs::canonical(sourcePath)),
    mExpectedResult(expectedResultDir.string()) {
  Logger::info("Load Expected Result: {}", expectedResultDir);
  auto checkZero = mExpectedResult.checkPassedTCEmpty();
  if (checkZero) {
    throw InvalidArgumentException(fmt::format("No passed TC in Expected Result({0})", expectedResultDir.string()));
  }
}

MutationResult Evaluator::compare(const Mutant& mut, const std::filesystem::path& ActualResultDir,
                                  TestExecutionState testState) {
  std::string killingTC;
  std::string errorTC;
  MutationState state;

  switch (testState) {
    case TestExecutionState::BUILD_FAILURE:
      state = MutationState::BUILD_FAILURE;
      Logger::verbose("build failure ({})", ActualResultDir);
      break;
    case TestExecutionState::TIMEOUT:
      state = MutationState::TIMEOUT;
      Logger::verbose("timeout ({})", ActualResultDir);
      break;
    case TestExecutionState::UNCOVERED:
      state = MutationState::SURVIVED;
      Logger::verbose("uncovered by tests - survived");
      break;
    case TestExecutionState::SUCCESS: {
      Result mActualResult(ActualResultDir.string());
      Logger::verbose("comparing results: {}", ActualResultDir);
      state = Result::compare(mExpectedResult, mActualResult, &killingTC, &errorTC);
      break;
    }
  }
  Logger::verbose("mutant: {}", mut.str());
  Logger::verbose("killing TC: {}", killingTC);
  Logger::verbose("error TC: {}", errorTC);
  Logger::verbose("state: {}", MutationStateToStr(state));

  fs::path relPath = fs::canonical(mut.getPath()).lexically_relative(mCanonicalSourcePath);

  std::size_t flen = 60;
  std::string mutLoc = fmt::format("{path} ({sl}:{sc}-{el}:{ec} -> {mc})", fmt::arg("path", relPath.string()),
                                   fmt::arg("sl", mut.getFirst().line), fmt::arg("sc", mut.getFirst().column),
                                   fmt::arg("el", mut.getLast().line), fmt::arg("ec", mut.getLast().column),
                                   fmt::arg("mc", mut.getToken().empty() ? "DELETE STMT" : mut.getToken()));

  int filePos = static_cast<int>(mutLoc.size()) - static_cast<int>(flen);
  std::string skipStr;
  if (filePos < 0) {
    filePos = 0;
  } else if (filePos > 1) {
    filePos += 4;
    skipStr = "... ";
  }

  Logger::verbose("{mu:>5} : {loc:.<{flen}} {status}", fmt::arg("mu", mut.getOperator()),
                   fmt::arg("loc", skipStr + mutLoc.substr(filePos)), fmt::arg("flen", flen),
                   fmt::arg("status", MutationStateToStr(state)));

  MutationResult ret(mut, killingTC, errorTC, state);

  return ret;
}

MutationResult Evaluator::compareAndSaveMutationResult(const Mutant& mut, const std::filesystem::path& ActualResultDir,
                                                       const std::filesystem::path& evalFilePath,
                                                       TestExecutionState testState) {
  auto outDir = evalFilePath.parent_path();
  io::ensureDirectoryExists(outDir);

  MutationResult ret = compare(mut, ActualResultDir, testState);

  std::ofstream outFile(evalFilePath.string(), std::ios::out | std::ios::app);
  outFile << ret << "\n";
  outFile.close();

  Logger::verbose("saved mutation result: {}", outDir);

  return ret;
}

}  // namespace sentinel
