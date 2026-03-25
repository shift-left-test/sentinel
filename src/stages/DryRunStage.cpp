/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <utility>
#include "sentinel/Console.hpp"
#include "sentinel/stages/DryRunStage.hpp"

namespace sentinel {

DryRunStage::DryRunStage(const Config& cfg, std::shared_ptr<StatusLine> sl, std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool DryRunStage::shouldSkip() const {
  return !mConfig.dryRun;
}

StatusLine::Phase DryRunStage::getPhase() const {
  return StatusLine::Phase::GENERATION;
}

bool DryRunStage::execute() {
  mStatusLine->setTotalMutants(mWorkspace->loadMutants().size());
  Console::out("Evaluation skipped (dry run).");
  return false;
}

}  // namespace sentinel
