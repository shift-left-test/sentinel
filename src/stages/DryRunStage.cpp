/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include "sentinel/Logger.hpp"
#include "sentinel/stages/DryRunStage.hpp"

namespace sentinel {

bool DryRunStage::shouldSkip(const PipelineContext& ctx) const {
  return !ctx.config.dryRun;
}

StatusLine::Phase DryRunStage::getPhase() const {
  return StatusLine::Phase::GENERATION;
}

bool DryRunStage::execute(PipelineContext* ctx) {
  ctx->statusLine.setProgressTotal(ctx->workspace.loadMutants().size());
  Logger::info("Evaluation skipped (dry run).");
  return false;
}

}  // namespace sentinel
