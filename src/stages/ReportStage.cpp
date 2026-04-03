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
  MutationResults results;
  for (const auto& [id, m] : ctx->workspace.loadMutants()) {
    results.push_back(ctx->workspace.getDoneResult(id));
  }

  MutationSummary summary(results, ctx->config.sourceDir);
  XmlReport xmlReport(summary);
  xmlReport.printSummary();
  if (!ctx->config.outputDir.empty()) {
    xmlReport.save(ctx->config.outputDir);
    HtmlReport(summary, ctx->config).save(ctx->config.outputDir);
    Logger::info("Reports saved to {}", ctx->config.outputDir);
  }

  std::optional<double> score;
  if (summary.totNumberOfMutation > 0) {
    score = 100.0 * static_cast<double>(summary.totNumberOfDetectedMutation) /
            static_cast<double>(summary.totNumberOfMutation);
  }

  std::string scoreStr = score ? fmt::format("{:.1f}%", *score) : "-";
  if (ctx->config.threshold) {
    bool passed = !score || *score >= *ctx->config.threshold;
    auto icon = passed ? Utf8Char::CheckMark : Utf8Char::CrossMark;
    std::string msg = fmt::format("Mutation testing complete {} {} {} (threshold: {:.1f}%)",
                                  Utf8Char::EmDash, scoreStr, icon, *ctx->config.threshold);
    if (passed) {
      Logger::info("{}", msg);
    } else {
      Logger::error("{}", msg);
      throw ThresholdError(*score, *ctx->config.threshold);
    }
  } else {
    Logger::info("Mutation testing complete {} {}", Utf8Char::EmDash, scoreStr);
  }
  return false;
}

}  // namespace sentinel
