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

namespace sentinel {

namespace fs = std::filesystem;

Report::Report(const MutationResults& results, const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mResults(results), mLogger(Logger::getLogger("Report")) {
  generateReport();
}

Report::Report(const std::filesystem::path& resultsPath, const std::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mLogger(Logger::getLogger("Report")) {
  mResults.load(resultsPath.string());
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
    fs::path curDirpath = mrPath.parent_path();

    groupByDirPath[curDirpath].results.push_back(&mr);
    groupByDirPath[curDirpath].total += 1;

    groupByPath[mrPath].results.push_back(&mr);
    groupByPath[mrPath].total += 1;

    if (mr.getDetected()) {
      groupByDirPath[curDirpath].detected += 1;
      groupByPath[mrPath].detected += 1;
      totNumberOfDetectedMutation += 1;
    }
  }

  for (auto& p : groupByDirPath) {
    std::set<fs::path> tmpSet;
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
      curScore = (100.0 * static_cast<double>(p.second.detected)) / static_cast<double>(p.second.total);
    }
    int filePos = static_cast<int>(p.first.string().size()) - static_cast<int>(flen);
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
    finalScore = (100.0 * static_cast<double>(totNumberOfDetectedMutation)) / static_cast<double>(totNumberOfMutation);
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

std::filesystem::path Report::getRelativePath(const std::filesystem::path& path,
                                              const std::filesystem::path& start) {
  return fs::canonical(path).lexically_relative(fs::canonical(start));
}

}  // namespace sentinel
