/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Console.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Report.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kReportWidth = 76;

static std::string repeatUtf8(const char* ch, std::size_t width) {
  std::string unit(ch);
  std::string result;
  result.reserve(unit.size() * width);
  for (std::size_t i = 0; i < width; ++i) {
    result += unit;
  }
  return result;
}

Report::Report(const MutationSummary& summary) :
    mSummary(summary) {
}

void Report::printSummary() const {
  const std::string thick = repeatUtf8("\xe2\x94\x81", kReportWidth);   // ━
  const std::string thin = repeatUtf8("\xe2\x94\x80", kReportWidth);    // ─

  static constexpr std::size_t flen = 40;
  static constexpr std::size_t klen = 8;
  static constexpr std::size_t slen = 10;
  static constexpr std::size_t mlen = 8;
  static constexpr std::size_t clen = 8;
  std::string rowFmt = "  {0:<{1}}{2:>{3}}{4:>{5}}{6:>{7}}{8:>{9}}";

  // Header
  Console::out("{}", thick);
  Console::out("{:^{}}", "Mutation Coverage Report", kReportWidth);
  Console::out("{}", thick);
  Console::out(rowFmt, "File", flen, "Killed", klen, "Survived", slen, "Total", mlen, "Score", clen);
  Console::out("{}", thin);

  // Per-file rows
  for (const auto& [path, stats] : mSummary.groupByPath) {
    double curScore = -1.0;
    if (stats.total != 0) {
      curScore = (100.0 * static_cast<double>(stats.detected)) / static_cast<double>(stats.total);
    }
    std::string filePath = path.string();
    int overflow = static_cast<int>(filePath.size()) - static_cast<int>(flen);
    std::string skipStr;
    if (overflow > 1) {
      filePath = filePath.substr(overflow + 4);
      skipStr = "... ";
    }
    std::string scoreStr = curScore >= 0.0 ? fmt::format("{:.1f}%", curScore) : "-%";
    std::size_t survived = stats.total - stats.detected;
    Console::out(rowFmt, skipStr + filePath, flen, stats.detected, klen, survived, slen,
                 stats.total, mlen, scoreStr, clen);
  }

  // Footer
  Console::out("{}", thick);
  double finalScore = -1.0;
  if (mSummary.totNumberOfMutation != 0) {
    finalScore = (100.0 * static_cast<double>(mSummary.totNumberOfDetectedMutation)) /
                 static_cast<double>(mSummary.totNumberOfMutation);
  }
  std::string finalScoreStr = finalScore >= 0.0 ? fmt::format("{:.1f}%", finalScore) : "-%";
  std::size_t totalSurvived = mSummary.totNumberOfMutation - mSummary.totNumberOfDetectedMutation;
  Console::out(rowFmt, "TOTAL", flen, mSummary.totNumberOfDetectedMutation, klen, totalSurvived, slen,
               mSummary.totNumberOfMutation, mlen, finalScoreStr, clen);

  // Skipped
  std::size_t totalSkipped =
      mSummary.totNumberOfBuildFailure + mSummary.totNumberOfRuntimeError + mSummary.totNumberOfTimeout;
  if (totalSkipped != 0) {
    Console::out("{}", thin);
    std::string skipped;
    if (mSummary.totNumberOfBuildFailure > 0) {
      skipped += fmt::format("{} build failure{}", mSummary.totNumberOfBuildFailure,
                             mSummary.totNumberOfBuildFailure == 1 ? "" : "s");
    }
    if (mSummary.totNumberOfRuntimeError > 0) {
      if (!skipped.empty()) skipped += ", ";
      skipped += fmt::format("{} runtime error{}", mSummary.totNumberOfRuntimeError,
                             mSummary.totNumberOfRuntimeError == 1 ? "" : "s");
    }
    if (mSummary.totNumberOfTimeout > 0) {
      if (!skipped.empty()) skipped += ", ";
      skipped += fmt::format("{} timeout{}", mSummary.totNumberOfTimeout,
                             mSummary.totNumberOfTimeout == 1 ? "" : "s");
    }
    Console::out("  Skipped: {}", skipped);
  }
  Console::out("{}", thick);
}

}  // namespace sentinel
