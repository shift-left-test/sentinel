/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Console.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

Report::Report(const MutationSummary& summary) :
    mSummary(summary) {
}

void Report::printSummary() const {
  std::size_t flen = 50;
  std::size_t klen = 10;
  std::size_t mlen = 10;
  std::size_t clen = 10;
  std::size_t maxlen = flen + klen + mlen + clen + 2;

  int cnt = 0;
  Logger::verbose("# of MutationResults: {}", mSummary.results.size());
  for (const MutationResult& mr : mSummary.results) {
    Logger::verbose("MutationResult #{}", ++cnt);
    Logger::verbose("  Mutant: {}", mr.getMutant().str());
    Logger::verbose("  killing TC: {}", mr.getKillingTest());
    Logger::verbose("  error TC: {}", mr.getErrorTest());
    Logger::verbose("  Mutation State: {}", MutationStateToStr(mr.getMutationState()));
  }

  std::string defFormat = "{0:<{1}}{2:>{3}}{4:>{5}}{6:>{7}}";
  Console::out("{0:=^{1}}", "", maxlen);
  Console::out(string::rtrim(fmt::format("{0:^{1}}", "Mutation Coverage Report", maxlen)));
  Console::out("{0:=^{1}}", "", maxlen);
  Console::out(defFormat, "File", flen, "Killed", klen, "Total", mlen, "Score", clen);
  Console::out("{0:-^{1}}", "", maxlen);

  for (const auto& p : mSummary.groupByPath) {
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
    Console::out(defFormat, skipStr + p.first.string().substr(filePos), flen, p.second.detected, klen, p.second.total,
                 mlen, scoreStr, clen);
  }
  Console::out("{0:-^{1}}", "", maxlen);

  double finalScore = -1.0;
  if (mSummary.totNumberOfMutation != 0) {
    finalScore = (100.0 * static_cast<double>(mSummary.totNumberOfDetectedMutation)) /
                 static_cast<double>(mSummary.totNumberOfMutation);
  }
  std::string finalScoreStr = finalScore >= 0.0 ? fmt::format("{:.1f}%", finalScore) : "-%";
  Console::out(defFormat, "TOTAL", flen, mSummary.totNumberOfDetectedMutation, klen, mSummary.totNumberOfMutation,
               mlen, finalScoreStr, clen);
  Console::out("{0:=^{1}}", "", maxlen);

  if ((mSummary.totNumberOfBuildFailure + mSummary.totNumberOfRuntimeError + mSummary.totNumberOfTimeout) != 0) {
    std::string skipped;
    if (mSummary.totNumberOfBuildFailure > 0) {
      skipped +=
          fmt::format("{} build failure{}", mSummary.totNumberOfBuildFailure,
                      mSummary.totNumberOfBuildFailure == 1 ? "" : "s");
    }
    if (mSummary.totNumberOfRuntimeError > 0) {
      if (!skipped.empty()) skipped += "  \xc2\xb7  ";
      skipped +=
          fmt::format("{} runtime error{}", mSummary.totNumberOfRuntimeError,
                      mSummary.totNumberOfRuntimeError == 1 ? "" : "s");
    }
    if (mSummary.totNumberOfTimeout > 0) {
      if (!skipped.empty()) skipped += "  \xc2\xb7  ";
      skipped +=
          fmt::format("{} timeout{}", mSummary.totNumberOfTimeout, mSummary.totNumberOfTimeout == 1 ? "" : "s");
    }
    Console::out("Skipped: {}", skipped);
    Console::out("{0:=^{1}}", "", maxlen);
  }
}

}  // namespace sentinel
