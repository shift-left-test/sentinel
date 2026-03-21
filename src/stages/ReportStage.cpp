/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <utility>
#include "sentinel/Evaluator.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/stages/ReportStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

ReportStage::ReportStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                         std::shared_ptr<Logger> log,
                         std::shared_ptr<Workspace> workspace)
    : Stage(cfg, std::move(sl), std::move(log)), mWorkspace(std::move(workspace)) {}

bool ReportStage::execute() {
  mStatusLine->setPhase(StatusLine::Phase::REPORT);

  Evaluator evaluator(mWorkspace->getOriginalResultsDir(), *mConfig.sourceDir);
  for (const auto& [id, m] : mWorkspace->loadMutants()) {
    evaluator.injectResult(mWorkspace->getDoneResult(id));
  }

  XMLReport xmlReport(evaluator.getMutationResults(), *mConfig.sourceDir);
  xmlReport.printSummary();
  if (mConfig.outputDir && !mConfig.outputDir->empty()) {
    xmlReport.save(*mConfig.outputDir);
    HTMLReport htmlReport(evaluator.getMutationResults(), *mConfig.sourceDir);
    htmlReport.save(*mConfig.outputDir);
  }

  mStatusLine->disable();
  clearSignalStatusLine();

  // Compute mutation score and check threshold.
  if (mConfig.threshold) {
    const auto& results = evaluator.getMutationResults();
    std::size_t total = results.size();
    if (total > 0) {
      std::size_t killed = static_cast<std::size_t>(
          std::count_if(results.begin(), results.end(),
                        [](const MutationResult& r) { return r.getDetected(); }));
      double score = 100.0 * static_cast<double>(killed) / static_cast<double>(total);
      if (score < *mConfig.threshold) {
        throw ThresholdError(score, *mConfig.threshold);
      }
    }
  }
  return false;
}

}  // namespace sentinel
