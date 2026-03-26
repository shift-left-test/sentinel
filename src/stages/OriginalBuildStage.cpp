/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/stages/OriginalBuildStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

OriginalBuildStage::OriginalBuildStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                                       std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool OriginalBuildStage::shouldSkip() const {
  return fs::exists(mWorkspace->getOriginalBuildLog());
}

StatusLine::Phase OriginalBuildStage::getPhase() const {
  return StatusLine::Phase::BUILD_ORIG;
}

bool OriginalBuildStage::execute() {
  Logger::info("Running original build...");
  fs::path buildLog = mWorkspace->getOriginalBuildLog();
  Subprocess buildProc(*mConfig.buildCmd, 0, 0, buildLog.string(), *mConfig.silent);
  buildProc.execute();
  if (!buildProc.isSuccessfulExit()) {
    throw std::runtime_error(fmt::format("Original build failed. See: {}", buildLog.string()));
  }
  if (!fs::exists(*mConfig.compileDbDir / "compile_commands.json")) {
    throw std::runtime_error(
        fmt::format("compile_commands.json not found in '{}'. "
                    "Make sure build-command generates it (e.g., add -DCMAKE_EXPORT_COMPILE_COMMANDS=ON to cmake).",
                    mConfig.compileDbDir->string()));
  }
  return true;
}

}  // namespace sentinel
