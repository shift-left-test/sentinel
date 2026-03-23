/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/DryRunStage.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static void printDryRunSummary(const Config& cfg, std::size_t computedTimeLimit,
                               const std::vector<std::pair<int, Mutant>>& indexedMutants, std::size_t candidateCount,
                               const fs::path& workspaceDir, std::size_t partIdx, std::size_t partCount) {
  Console::out("\n=== Sentinel Dry Run ===");
  Console::out("  source-dir:    {}", cfg.sourceDir->string());
  Console::out("  build-command: {}", cfg.buildCmd.value_or(""));
  Console::out("  test-command:  {}", cfg.testCmd.value_or(""));
  Console::out("  scope:         {}", cfg.scope.value_or("all"));
  Console::out("  limit:         {}", cfg.limit.value_or(0) == 0 ? "unlimited" : std::to_string(*cfg.limit));
  Console::out("  operators:     {}", cfg.operators->empty() ? "all" : sentinel::string::join(", ", *cfg.operators));
  if (partIdx != 0) {
    Console::out("  partition:     {}/{}", partIdx, partCount);
  }
  Console::out("  workspace:     {}", workspaceDir.string());

  Console::out("\n  {}  Original build", "[ OK ]");  // Baseline run already happened

  if (cfg.timeout == "auto") {
    Console::out("  {}  Original tests  (auto-timeout: {}s)", "[ OK ]", computedTimeLimit);
  } else {
    Console::out("  {}  Original tests  (timeout: {}s)", "[ OK ]", computedTimeLimit);
  }

  if (indexedMutants.empty()) {
    Console::out("  [WARN]  Mutants: 0 — nothing to evaluate.");
  } else {
    Console::out("  [ OK ]  Mutants: {} of {} candidates", indexedMutants.size(), candidateCount);
  }

  if (cfg.verbose && *cfg.verbose) {
    for (const auto& [id, m] : indexedMutants) {
      Console::out("          [{:3d}] {} @ {}:{}", id, m.getOperator(), m.getPath().filename().string(),
                   m.getFirst().line);
    }
  }

  if (!indexedMutants.empty()) {
    Console::out("\nWorkspace saved. Remove --dry-run to start mutation testing");
  }
}

DryRunStage::DryRunStage(const Config& cfg, std::shared_ptr<StatusLine> sl, std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool DryRunStage::execute() {
  if (!mConfig.dryRun) return true;

  auto indexedMutants = mWorkspace->loadMutants();
  auto status = mWorkspace->loadStatus();

  std::size_t computedTimeLimit = 0;
  if (*mConfig.timeout == "auto") {
    computedTimeLimit = status.baselineTime.value_or(0);
  } else {
    computedTimeLimit = std::stoul(*mConfig.timeout);
  }
  std::size_t candidateCount = status.candidateCount.value_or(indexedMutants.size());
  std::size_t partIdx = status.partIndex.value_or(0);
  std::size_t partCount = status.partCount.value_or(0);

  mStatusLine->setTotalMutants(indexedMutants.size());
  mStatusLine->disable();

  printDryRunSummary(mConfig, computedTimeLimit, indexedMutants, candidateCount, mWorkspace->getRoot(), partIdx,
                     partCount);
  return false;
}

}  // namespace sentinel
