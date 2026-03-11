/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::filesystem;

namespace sentinel {

Report::Report(const MutationResults& results, const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mResults(results), mLogger(Logger::getLogger("Report")) {
  generateReport();
}

Report::Report(const std::filesystem::path& resultsPath,
               const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mLogger(Logger::getLogger("Report")) {
  mResults.load(resultsPath);
  mLogger->verbose("Load MutationResults: {}", resultsPath.string());

  generateReport();
}

void Report::generateReport() {
  if (!fs::exists(mSourcePath) || !fs::is_directory(mSourcePath)) {
    throw InvalidArgumentException(fmt::format("source path does not exist: {}", mSourcePath.string()));
  }

  for (const MutationResult& mr : mResults) {
    auto currentState = mr.getMutationState();
    if (currentState == MutationState::BUILD_FAILURE) {
      totNumberOfBuildFailure++;
      continue;
    }
    if (currentState == MutationState::RUNTIME_ERROR) {
      totNumberOfRuntimeError++;
      continue;
    }
    if (currentState == MutationState::TIMEOUT) {
      totNumberOfTimeout++;
      continue;
    }
    totNumberOfMutation++;

    fs::path mrPath = getRelativePath(mr.getMutant().getPath(), mSourcePath);
    std::string curDirname = mrPath.parent_path();
    curDirname = string::replaceAll(curDirname, "/", ".");

    groupByDirPath[curDirname].results.push_back(&mr);
    groupByDirPath[curDirname].total += 1;

    groupByPath[mrPath].results.push_back(&mr);
    groupByPath[mrPath].total += 1;

    if (mr.getDetected()) {
      groupByDirPath[curDirname].detected += 1;
      groupByPath[mrPath].detected += 1;
      totNumberOfDetectedMutation += 1;
    }
  }

  for (auto& p : groupByDirPath) {
    std::set<std::string> tmpSet;
    for (const MutationResult* mr : p.second.results) {
      tmpSet.insert(mr->getMutant().getPath());
    }
    p.second.fileCount = tmpSet.size();
  }
}

void Report::printSummary() const {
  std::size_t flen = 50;
  std::size_t klen = 10;
  std::size_t mlen = 10;
  std::size_t clen = 10;
  std::size_t maxlen = flen + klen + mlen + clen + 2;

  int cnt = 0;
  mLogger->verbose("# of MutationResults: {}", mResults.size());
  for (const MutationResult& mr : mResults) {
    mLogger->verbose("MutationResult #{}", ++cnt);
    mLogger->verbose("  Mutant: {}", mr.getMutant().str());
    mLogger->verbose("  killing TC: {}", mr.getKillingTest());
    mLogger->verbose("  error TC: {}", mr.getErrorTest());
    mLogger->verbose("  Mutation State: {}", MutationStateToStr(mr.getMutationState()));
  }

  std::string defFormat = "{0:<{1}}{2:>{3}}{4:>{5}}{6:>{7}}";
  Console::out("{0:=^{1}}", "", maxlen);
  Console::out(string::rtrim(fmt::format("{0:^{1}}", "Mutation Coverage Report", maxlen)));
  Console::out("{0:=^{1}}", "", maxlen);
  Console::out(defFormat, "File", flen, "Killed", klen, "Total", mlen, "Score", clen);
  Console::out("{0:-^{1}}", "", maxlen);

  for (const auto& p : groupByPath) {
    double curScore = -1.0;
    if (p.second.total != 0) {
      curScore = (100.0 * p.second.detected) / p.second.total;
    }
    int filePos = p.first.string().size() - flen;
    std::string skipStr;
    if (filePos < 0) {
      filePos = 0;
    } else if (filePos > 1) {
      filePos += 4;
      skipStr = "... ";
    }
    std::string scoreStr = curScore >= 0.0 ? fmt::format("{:.1f}%", curScore) : "-%";
    Console::out(defFormat, skipStr + p.first.string().substr(filePos), flen,
                 p.second.detected, klen, p.second.total, mlen,
                 scoreStr, clen);
  }
  Console::out("{0:-^{1}}", "", maxlen);

  double finalScore = -1.0;
  if (totNumberOfMutation != 0) {
    finalScore = (100.0 * totNumberOfDetectedMutation) / totNumberOfMutation;
  }
  std::string finalScoreStr = finalScore >= 0.0 ? fmt::format("{:.1f}%", finalScore) : "-%";
  Console::out(defFormat, "TOTAL", flen, totNumberOfDetectedMutation, klen,
               totNumberOfMutation, mlen, finalScoreStr, clen);
  Console::out("{0:=^{1}}", "", maxlen);

  if ((totNumberOfBuildFailure + totNumberOfRuntimeError + totNumberOfTimeout) != 0) {
    std::string skipped;
    if (totNumberOfBuildFailure > 0) {
      skipped += fmt::format("{} build failure{}", totNumberOfBuildFailure,
                             totNumberOfBuildFailure == 1 ? "" : "s");
    }
    if (totNumberOfRuntimeError > 0) {
      if (!skipped.empty()) skipped += "  \xc2\xb7  ";
      skipped += fmt::format("{} runtime error{}", totNumberOfRuntimeError,
                             totNumberOfRuntimeError == 1 ? "" : "s");
    }
    if (totNumberOfTimeout > 0) {
      if (!skipped.empty()) skipped += "  \xc2\xb7  ";
      skipped += fmt::format("{} timeout{}", totNumberOfTimeout,
                             totNumberOfTimeout == 1 ? "" : "s");
    }
    Console::out("Skipped: {}", skipped);
    Console::out("{0:=^{1}}", "", maxlen);
  }
}

fs::path Report::getRelativePath(const std::string& path, const std::string& start) {
  // 1. convert p and base to absolute paths
  auto p = fs::canonical(path);
  auto base = fs::canonical(start);

  // 2. find first mismatch and shared root path
  auto mismatched = std::mismatch(p.begin(), p.end(), base.begin(), base.end());

  // 3. if no mismatch return "."
  if (mismatched.first == p.end() && mismatched.second == base.end()) {
    return ".";
  }

  auto it_p = mismatched.first;
  auto it_base = mismatched.second;

  fs::path ret;

  // 4. iterate abase to the shared root and append "../"
  for (; it_base != base.end(); ++it_base) {
    if (!it_base->empty()) {
      ret /= "..";
    }
  }

  // 5. iterate from the shared root to the p and append its parts
  for (; it_p != p.end(); ++it_p) {
    ret /= *it_p;
  }
  return ret;
}

}  // namespace sentinel
