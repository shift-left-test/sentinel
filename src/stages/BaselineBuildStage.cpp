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
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/stages/BaselineBuildStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

BaselineBuildStage::BaselineBuildStage(const Config& cfg, StatusLine& sl,
                                       std::shared_ptr<Logger> log, fs::path workDir)
    : Stage(cfg, sl, std::move(log)), mWorkDir(std::move(workDir)) {}

bool BaselineBuildStage::execute() {
  fs::path buildLog = mWorkDir / "original" / "build.log";
  if (fs::exists(buildLog)) return true;  // already done

  mStatusLine.setPhase(StatusLine::Phase::BUILD_ORIG);
  Subprocess buildProc(*mConfig.buildCmd, 0, 0, buildLog.string(), *mConfig.silent);
  buildProc.execute();
  if (!buildProc.isSuccessfulExit()) {
    throw std::runtime_error(
        fmt::format("Baseline build failed. See: {}", buildLog.string()));
  }
  if (!fs::exists(*mConfig.compileDbDir / "compile_commands.json")) {
    throw std::runtime_error(fmt::format(
        "compile_commands.json not found in '{}'. "
        "Make sure build-command generates it (e.g., add -DCMAKE_EXPORT_COMPILE_COMMANDS=ON to cmake).",
        mConfig.compileDbDir->string()));
  }
  return true;
}

}  // namespace sentinel
