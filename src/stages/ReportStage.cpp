/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <utility>
#include "sentinel/HtmlReport.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XmlReport.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"
#include "sentinel/stages/ReportStage.hpp"

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
  }

  // Compute mutation score and check threshold.
  if (mConfig.threshold) {
    std::size_t total = results.size();
    if (total > 0) {
      std::size_t killed = static_cast<std::size_t>(
          std::count_if(results.begin(), results.end(), [](const MutationResult& r) { return r.getDetected(); }));
      double score = 100.0 * static_cast<double>(killed) / static_cast<double>(total);
      if (score < *mConfig.threshold) {
        throw ThresholdError(score, *mConfig.threshold);
      }
    }
  }
  return false;
}

}  // namespace sentinel
