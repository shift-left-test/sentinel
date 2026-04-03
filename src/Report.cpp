/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <cmath>
#include <filesystem>  // NOLINT
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/util/Utf8Char.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kReportWidth = 80;

static std::string stateToLabel(MutationState state) {
  switch (state) {
    case MutationState::KILLED: return "Killed";
    case MutationState::SURVIVED: return "Survived";
    case MutationState::BUILD_FAILURE: return "Build Failure";
    case MutationState::RUNTIME_ERROR: return "Runtime Error";
    case MutationState::TIMEOUT: return "Timeout";
    default: return "Unknown";
  }
}

Report::~Report() = default;

Report::Report(const MutationSummary& summary) :
    mSummary(summary) {
}

void Report::printSummary() const {
  const std::string thick = Utf8Char::ThickLine * kReportWidth;
  const std::string thin = Utf8Char::ThinLine * kReportWidth;

  static constexpr std::size_t klen = 8;
  static constexpr std::size_t slen = 10;
  static constexpr std::size_t mlen = 8;
  static constexpr std::size_t clen = 8;
  static constexpr std::size_t flen = kReportWidth - klen - slen - mlen - clen - 2;

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
    std::string filePath = string::truncate(path.string(), flen);
    std::string scoreStr = curScore >= 0.0 ? fmt::format("{:.1f}%", curScore) : "-%";
    std::size_t survived = stats.total - stats.detected;
    Console::out(rowFmt, filePath, flen, stats.detected, klen, survived, slen,
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
  // Duration section
  if (mSummary.timedMutantCount > 0) {
    Console::out("{}", thin);

    std::size_t totalMutants = mSummary.totNumberOfMutation + mSummary.totNumberOfBuildFailure +
                               mSummary.totNumberOfRuntimeError + mSummary.totNumberOfTimeout;
    double totalTimeSecs = mSummary.totalBuildSecs + mSummary.totalTestSecs;

    // State rows have a 27-char prefix ("    " + 17-char label + 3-char pct + "%  ").
    // A 10-char time field accommodates values like "10h 30m".
    static constexpr std::size_t kTimeEndCol = 37;
    static constexpr std::size_t kStatePrefixLen = 27;  // 4 + 17 + 3 + 3
    static constexpr std::size_t kStateTimeWidth = kTimeEndCol - kStatePrefixLen;

    // Header line — align "100%" with per-state "XX%" column
    std::string prefix = (mSummary.timedMutantCount < totalMutants)
        ? fmt::format("  Duration ({}/{} mutants):", mSummary.timedMutantCount, totalMutants)
        : std::string("  Duration:");
    static constexpr std::size_t kPctCol = 21;  // column where pct digits start (4 + 17)
    std::size_t padLen = prefix.size() < kPctCol ? kPctCol - prefix.size() : 1;
    Console::out("{}{}100%  {:>{}s}  [{}/{}]",
                 prefix, std::string(padLen, ' '),
                 Timestamper::format(totalTimeSecs), kStateTimeWidth,
                 Timestamper::format(mSummary.totalBuildSecs),
                 Timestamper::format(mSummary.totalTestSecs));

    // Collect states with timed mutants, sort by total time descending
    std::vector<std::pair<MutationState, MutationSummary::StateTiming>> timedStates;
    for (const auto& [state, timing] : mSummary.timeByState) {
      if (timing.timedCount > 0) {
        timedStates.emplace_back(state, timing);
      }
    }
    std::sort(timedStates.begin(), timedStates.end(),
              [](const auto& a, const auto& b) {
                return (a.second.buildSecs + a.second.testSecs) >
                       (b.second.buildSecs + b.second.testSecs);
              });

    // Print each state row
    for (const auto& [state, timing] : timedStates) {
      double stateTotal = timing.buildSecs + timing.testSecs;
      double pct = std::round((stateTotal / totalTimeSecs) * 100.0);
      Console::out("    {:<17s}{:>3.0f}%  {:>{}s}  [{}/{}]",
                   stateToLabel(state), pct,
                   Timestamper::format(stateTotal), kStateTimeWidth,
                   Timestamper::format(timing.buildSecs),
                   Timestamper::format(timing.testSecs));
    }
  }

  Console::out("{}", thick);
}

}  // namespace sentinel
