/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include "sentinel/Console.hpp"
#include "sentinel/HtmlReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XmlReport.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"
#include "sentinel/stages/ReportStage.hpp"
#include "sentinel/util/Utf8Char.hpp"

namespace sentinel {

namespace fs = std::filesystem;

ReportStage::ReportStage(const Config& cfg, std::shared_ptr<StatusLine> sl, std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool ReportStage::shouldSkip() const {
  return false;
}

StatusLine::Phase ReportStage::getPhase() const {
  return StatusLine::Phase::REPORT;
}

bool ReportStage::execute() {
  Logger::info("Generating report...");
  MutationResults results;
  for (const auto& [id, m] : mWorkspace->loadMutants()) {
    results.push_back(mWorkspace->getDoneResult(id));
  }

  MutationSummary summary(results, *mConfig.sourceDir);
  XmlReport xmlReport(summary);
  xmlReport.printSummary();
  if (mConfig.outputDir && !mConfig.outputDir->empty()) {
    xmlReport.save(*mConfig.outputDir);
    HtmlReport(summary).save(*mConfig.outputDir);
    Logger::info("Reports saved to {}", *mConfig.outputDir);
  }

  // Compute mutation score for summary and threshold check.
  const std::size_t total = results.size();
  std::optional<double> score;
  if (total > 0) {
    const std::size_t killed = static_cast<std::size_t>(
        std::count_if(results.begin(), results.end(), [](const MutationResult& r) { return r.getDetected(); }));
    score = 100.0 * static_cast<double>(killed) / static_cast<double>(total);
  }

  // Print final summary line.
  std::string summaryLine = fmt::format("Mutation testing complete {} {} mutant{}",
                                        Utf8Char::EmDash, total, total == 1 ? "" : "s");
  if (score) {
    summaryLine += fmt::format(", score: {:.1f}%", *score);
  }
  if (mConfig.threshold) {
    auto icon = (score && *score >= *mConfig.threshold) ? Utf8Char::CheckMark : Utf8Char::CrossMark;
    summaryLine += fmt::format(" (threshold: {:.1f}% {})", *mConfig.threshold, icon);
  }
  Console::out("");
  Console::out("{}", summaryLine);

  if (mConfig.threshold && score && *score < *mConfig.threshold) {
    throw ThresholdError(*score, *mConfig.threshold);
  }
  return false;
}

}  // namespace sentinel
