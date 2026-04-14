/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <optional>
#include <string>
#include "sentinel/HtmlReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XmlReport.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"
#include "sentinel/stages/ReportStage.hpp"
#include "sentinel/util/Utf8Char.hpp"

namespace sentinel {

namespace fs = std::filesystem;

bool ReportStage::shouldSkip(const PipelineContext& ctx) const {
  (void)ctx;
  return false;
}

StatusLine::Phase ReportStage::getPhase() const {
  return StatusLine::Phase::REPORT;
}

bool ReportStage::execute(PipelineContext* ctx) {
  Logger::info("Generating report...");
  MutationResults results = ctx->workspace.loadResults();

  Config reportCfg = ctx->config;
  auto status = ctx->workspace.loadStatus();
  if (!reportCfg.seed.has_value()) {
    reportCfg.seed = status.seed;
  }
  if (!reportCfg.from.has_value() && status.from.has_value()) {
    reportCfg.from = status.from;
  }
  if (!reportCfg.uncommitted && status.uncommitted.has_value()) {
    reportCfg.uncommitted = *status.uncommitted;
  }
  if (reportCfg.limit == 0 && status.limit.has_value()) {
    reportCfg.limit = *status.limit;
  }

  MutationSummary summary(results, reportCfg.sourceDir);
  XmlReport xmlReport(summary);
  xmlReport.printSummary();
  if (!reportCfg.outputDir.empty()) {
    Logger::info("Writing reports to '{}'...", reportCfg.outputDir);
    xmlReport.save(reportCfg.outputDir);
    HtmlReport(summary, reportCfg).save(reportCfg.outputDir);
    Logger::info("Reports saved to '{}'", reportCfg.outputDir);
  }

  std::optional<double> score;
  if (summary.totNumberOfMutation > 0) {
    score = 100.0 * static_cast<double>(summary.totNumberOfDetectedMutation) /
            static_cast<double>(summary.totNumberOfMutation);
  }

  std::string scoreStr = score ? fmt::format("{:.1f}%", *score) : "-";
  if (reportCfg.threshold) {
    bool passed = !score || *score >= *reportCfg.threshold;
    auto icon = passed ? Utf8Char::CheckMark : Utf8Char::CrossMark;
    std::string msg = fmt::format("Mutation testing complete {} {} {} (threshold: {:.1f}%)",
                                  Utf8Char::EmDash, scoreStr, icon, *reportCfg.threshold);
    if (passed) {
      Logger::info("{}", msg);
    } else {
      Logger::error("{}", msg);
      throw ThresholdError(*score, *reportCfg.threshold);
    }
  } else {
    Logger::info("Mutation testing complete {} {}", Utf8Char::EmDash, scoreStr);
  }
  return false;
}

}  // namespace sentinel
