/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <optional>
#include <string>
#include <utility>
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

  MutationSummary summary(results, mConfig.sourceDir);
  XmlReport xmlReport(summary);
  xmlReport.printSummary();
  if (!mConfig.outputDir.empty()) {
    xmlReport.save(mConfig.outputDir);
    HtmlReport(summary).save(mConfig.outputDir);
    Logger::info("Reports saved to {}", mConfig.outputDir);
  }

  std::optional<double> score;
  if (summary.totNumberOfMutation > 0) {
    score = 100.0 * static_cast<double>(summary.totNumberOfDetectedMutation) /
            static_cast<double>(summary.totNumberOfMutation);
  }

  std::string scoreStr = score ? fmt::format("{:.1f}%", *score) : "-";
  if (mConfig.threshold) {
    bool passed = !score || *score >= *mConfig.threshold;
    auto icon = passed ? Utf8Char::CheckMark : Utf8Char::CrossMark;
    std::string msg = fmt::format("Mutation testing complete {} {} {} (threshold: {:.1f}%)",
                                  Utf8Char::EmDash, scoreStr, icon, *mConfig.threshold);
    if (passed) {
      Logger::info("{}", msg);
    } else {
      Logger::error("{}", msg);
      throw ThresholdError(*score, *mConfig.threshold);
    }
  } else {
    Logger::info("Mutation testing complete {} {}", Utf8Char::EmDash, scoreStr);
  }
  return false;
}

}  // namespace sentinel
