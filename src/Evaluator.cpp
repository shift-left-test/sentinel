/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <string>
#include "sentinel/Evaluator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"

namespace fs = std::filesystem;

namespace sentinel {

Evaluator::Evaluator(const std::string& expectedResultDir, const std::string& sourcePath) :
    mSourcePath(sourcePath), mCanonicalSourcePath(fs::canonical(sourcePath)),
    mExpectedResult(expectedResultDir), mLogger(Logger::getLogger("Evaluator")) {
  mLogger->info("Load Expected Result: {}", expectedResultDir);
  auto checkZero = mExpectedResult.checkPassedTCEmpty();
  if (checkZero) {
    throw InvalidArgumentException(fmt::format("No passed TC in Expected Result({0})", expectedResultDir));
  }
}

MutationResult Evaluator::compare(const Mutant& mut, const std::string& ActualResultDir, const std::string& testState) {
  std::string killingTC;
  std::string errorTC;
  MutationState state;

  if (testState == "build_failure") {
    state = MutationState::BUILD_FAILURE;
    mLogger->verbose("build failure ({})", ActualResultDir);
  } else if (testState == "timeout") {
    state = MutationState::TIMEOUT;
    mLogger->verbose("timeout ({})", ActualResultDir);
  } else if (testState == "uncovered") {
    state = MutationState::SURVIVED;
    mLogger->verbose("uncovered by tests - survived");
  } else if (testState == "success") {
    Result mActualResult(ActualResultDir);
    mLogger->verbose("comparing results: {}", ActualResultDir);
    state = Result::compare(mExpectedResult, mActualResult, &killingTC, &errorTC);
  } else {
    throw InvalidArgumentException(fmt::format("Invalid value for testState : {0}", testState));
  }
  mLogger->verbose("mutant: {}", mut.str());
  mLogger->verbose("killing TC: {}", killingTC);
  mLogger->verbose("error TC: {}", errorTC);
  mLogger->verbose("state: {}", MutationStateToStr(state));

  fs::path relPath;

  auto p = fs::canonical(mut.getPath());
  const auto& base = mCanonicalSourcePath;

  auto mismatched = std::mismatch(p.begin(), p.end(), base.begin(), base.end());

  if (mismatched.first == p.end() && mismatched.second == base.end()) {
    relPath /= ".";
  } else {
    auto it_p = mismatched.first;
    auto it_base = mismatched.second;

    for (; it_base != base.end(); ++it_base) {
      if (!it_base->empty()) {
        relPath /= "..";
      }
    }

    for (; it_p != p.end(); ++it_p) {
      relPath /= *it_p;
    }
  }

  std::size_t flen = 60;
  std::string mutLoc = fmt::format("{path} ({sl}:{sc}-{el}:{ec} -> {mc})", fmt::arg("path", relPath.string()),
                                   fmt::arg("sl", mut.getFirst().line), fmt::arg("sc", mut.getFirst().column),
                                   fmt::arg("el", mut.getLast().line), fmt::arg("ec", mut.getLast().column),
                                   fmt::arg("mc", mut.getToken().empty() ? "DELETE STMT" : mut.getToken()));

  int filePos = mutLoc.size() - flen;
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

MutationResult Evaluator::compareAndSaveMutationResult(const Mutant& mut,
                                                       const std::filesystem::path& ActualResultDir,
                                                       const std::filesystem::path& evalFilePath,
                                                       const std::string& testState) {
  auto outDir = evalFilePath.parent_path();
  if (fs::exists(outDir)) {
    if (!fs::is_directory(outDir)) {
      throw InvalidArgumentException(fmt::format("'{}' is not a directory", outDir.string()));
    }
  } else {
    fs::create_directories(outDir);
  }

  MutationResult ret = compare(mut, ActualResultDir, testState);

  std::ofstream outFile(evalFilePath.c_str(), std::ios::out | std::ios::app);
  outFile << ret << std::endl;
  outFile.close();

  mLogger->verbose("saved mutation result: {}", outDir.string());

  return ret;
}

}  // namespace sentinel
