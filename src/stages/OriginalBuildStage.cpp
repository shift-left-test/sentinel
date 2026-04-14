/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <stdexcept>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/stages/OriginalBuildStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kLogTailLines = 20;

bool OriginalBuildStage::shouldSkip(const PipelineContext& ctx) const {
  return fs::exists(ctx.workspace.getOriginalBuildLog());
}

StatusLine::Phase OriginalBuildStage::getPhase() const {
  return StatusLine::Phase::BUILD_ORIG;
}

bool OriginalBuildStage::execute(PipelineContext* ctx) {
  Logger::info("Running original build...");
  fs::path buildLog = ctx->workspace.getOriginalBuildLog();
  Logger::verbose("Build command: {}", ctx->config.buildCmd);
  Logger::verbose("Build log: {}", buildLog);
  Timestamper ts;
  Subprocess buildProc(ctx->config.buildCmd, 0, buildLog.string(), !isVerbose(*ctx));
  buildProc.execute();
  if (!buildProc.isSuccessfulExit()) {
    std::string msg = fmt::format("Original build failed.\n       See: {}", buildLog.string());
    if (!isVerbose(*ctx)) {
      io::appendLogTail(&msg, buildLog, kLogTailLines, "build output");
    }
    msg += fmt::format("\n\n       Build command: {}", ctx->config.buildCmd);
    throw std::runtime_error(msg);
  }
  Logger::info("Original build completed ({})", Timestamper::format(ts.toDouble()));
  return true;
}

}  // namespace sentinel
