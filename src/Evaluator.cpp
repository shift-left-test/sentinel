/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Result.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Evaluator::Evaluator(const std::filesystem::path& expectedResultDir,
                     const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mCanonicalSourcePath(fs::canonical(sourcePath)),
    mExpectedResult(expectedResultDir.string()), mLogger(Logger::getLogger("Evaluator")) {
  mLogger->info("Load Expected Result: {}", expectedResultDir.string());
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
      mLogger->verbose("build failure ({})", ActualResultDir.string());
      break;
    case TestExecutionState::TIMEOUT:
      state = MutationState::TIMEOUT;
      mLogger->verbose("timeout ({})", ActualResultDir.string());
      break;
    case TestExecutionState::UNCOVERED:
      state = MutationState::SURVIVED;
      mLogger->verbose("uncovered by tests - survived");
      break;
    case TestExecutionState::SUCCESS: {
      Result mActualResult(ActualResultDir.string());
      mLogger->verbose("comparing results: {}", ActualResultDir.string());
      state = Result::compare(mExpectedResult, mActualResult, &killingTC, &errorTC);
      break;
    }
  }
  mLogger->verbose("mutant: {}", mut.str());
  mLogger->verbose("killing TC: {}", killingTC);
  mLogger->verbose("error TC: {}", errorTC);
  mLogger->verbose("state: {}", MutationStateToStr(state));

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

  mLogger->verbose("{mu:>5} : {loc:.<{flen}} {status}",
                   fmt::arg("mu", mut.getOperator()),
                   fmt::arg("loc", skipStr + mutLoc.substr(filePos)),
                   fmt::arg("flen", flen),
                   fmt::arg("status", MutationStateToStr(state)));

  MutationResult ret(mut, killingTC, errorTC, state);

  mMutationResults.push_back(ret);

  return ret;
}

void Evaluator::injectResult(const MutationResult& result) {
  mMutationResults.push_back(result);
}

const MutationResults& Evaluator::getMutationResults() const {
  return mMutationResults;
}

MutationResult Evaluator::compareAndSaveMutationResult(const Mutant& mut,
                                                       const std::filesystem::path& ActualResultDir,
                                                       const std::filesystem::path& evalFilePath,
                                                       TestExecutionState testState) {
  auto outDir = evalFilePath.parent_path();
  if (fs::exists(outDir)) {
    if (!fs::is_directory(outDir)) {
      throw InvalidArgumentException(fmt::format("'{}' is not a directory", outDir.string()));
    }
  } else {
    fs::create_directories(outDir);
  }

  MutationResult ret = compare(mut, ActualResultDir, testState);

  std::ofstream outFile(evalFilePath.string(), std::ios::out | std::ios::app);
  outFile << ret << "\n";
  outFile.close();

  mLogger->verbose("saved mutation result: {}", outDir.string());

  return ret;
}

}  // namespace sentinel
