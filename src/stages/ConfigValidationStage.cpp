/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/stages/ConfigValidationStage.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

ConfigValidationStage::ConfigValidationStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                                             std::shared_ptr<Workspace> workspace)
    : Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {}

bool ConfigValidationStage::execute() {
  // Skip on resume or already-complete
  bool alreadyComplete = mWorkspace->isComplete();
  bool resuming = !alreadyComplete && mWorkspace->hasPreviousRun();
  if (alreadyComplete || resuming) return true;

  // Validate required options
  if (mConfig.buildCmd->empty()) {
    throw InvalidArgumentException("Option --build-command is required.");
  }
  if (mConfig.testCmd->empty()) {
    throw InvalidArgumentException("Option --test-command is required.");
  }
  if (mConfig.testResultDir->empty()) {
    throw InvalidArgumentException("Option --test-result-dir is required.");
  }

  // Validate threshold
  if (mConfig.threshold && (*mConfig.threshold < 0.0 || *mConfig.threshold > 100.0)) {
    throw InvalidArgumentException(
        fmt::format("Invalid --threshold value: {:.1f}. Expected a percentage in [0, 100].",
                    *mConfig.threshold));
  }

  // Validate partition
  if (mConfig.partition && !mConfig.partition->empty()) {
    try {
      Partition::parse(*mConfig.partition);
    } catch (const std::invalid_argument& e) {
      throw InvalidArgumentException(e.what());
    }
    if (!mConfig.seed) {
      throw InvalidArgumentException("--partition requires an explicit --seed value.");
    }
  }

  return checkWarnings();
}

bool ConfigValidationStage::checkWarnings() {
  std::vector<std::string> warnings;

  // limit=0: evaluates every candidate mutant, which may take hours.
  if (mConfig.limit && *mConfig.limit == 0) {
    warnings.push_back("limit: 0 - all candidate mutants will be evaluated. This may take a very long time.");
  }

  // timeout=0: no per-mutant time cap; a hanging test blocks the run forever.
  if (mConfig.timeout && *mConfig.timeout == "0") {
    warnings.push_back("timeout: 0 - no per-mutant test time limit. A hanging test will block the run indefinitely.");
  }

  // --exclude checks
  if (mConfig.excludes) {
    for (const auto& excl : *mConfig.excludes) {
      if (!excl.empty() && excl.back() == '/') {
        warnings.push_back(fmt::format("exclude: '{}' ends with '/'. "
                                       "Patterns are matched against file paths, not directories.", excl));
      } else if (!excl.empty() && excl.front() != '*' && !fs::path(excl).is_absolute()) {
        warnings.push_back(fmt::format("exclude: '{}' is a relative pattern without a leading '*'.", excl));
      }
    }
  }

  // --pattern checks
  if (mConfig.patterns) {
    fs::path srcRoot = *mConfig.sourceDir;

    for (const auto& pat : *mConfig.patterns) {
      fs::path patPath(pat);
      if (patPath.is_absolute()) {
        // sourceDir is canonical absolute
        auto rel = patPath.lexically_relative(srcRoot);
        if (rel.empty() || rel.native().find("..") != std::string::npos) {
          warnings.push_back(fmt::format("pattern: '{}' is an absolute path outside source-dir.", pat));
        } else {
          warnings.push_back(fmt::format("pattern: '{}' is an absolute path. "
                                         "Git pathspec uses paths relative to repository root.", pat));
        }
      }
    }
  }

  if (!warnings.empty() && !(mConfig.force && *mConfig.force)) {
    Console::out("\nConfiguration warnings:");
    for (const auto& w : warnings) {
      Console::out("  [!] {}", w);
    }
    if (!Console::confirm("\nProceed?")) {
      Console::out("Aborted.");
      return false;
    }
  }
  return true;
}

}  // namespace sentinel
