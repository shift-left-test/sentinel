/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <cmath>
#include <filesystem>  // NOLINT
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/stages/OriginalTestStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kAutoTimeoutPaddingSecs = 5;
static constexpr double kAutoTimeoutFactor = 1.5;

bool OriginalTestStage::shouldSkip(const PipelineContext& ctx) const {
  return fs::exists(ctx.workspace.getOriginalTestLog()) && ctx.workspace.hasPreviousRun();
}

StatusLine::Phase OriginalTestStage::getPhase() const {
  return StatusLine::Phase::TEST_ORIG;
}

bool OriginalTestStage::execute(PipelineContext* ctx) {
  Logger::info("Running original test...");
  fs::path testLog = ctx->workspace.getOriginalTestLog();
  Logger::verbose("Test command: {}", ctx->config.testCmd);
  Logger::verbose("Test log: {}", testLog);
  Logger::verbose("Test result dir: {}", ctx->config.testResultDir.string());

  std::size_t computedTimeLimit = 0;
  if (ctx->config.timeout.has_value()) {
    computedTimeLimit = *ctx->config.timeout;
    Logger::info("Timeout: {}", Timestamper::format(computedTimeLimit));
  }

  Timestamper testTimer;
  Subprocess testProc(ctx->config.testCmd, computedTimeLimit, testLog.string(),
                      !isVerbose(*ctx));
  testProc.execute();
  const double testElapsed = testTimer.toDouble();

  if (testProc.isSignaled() || testProc.isSignalExit()) {
    throw std::runtime_error(fmt::format(
        "Original test command was killed by a signal. See: {}",
        testLog.string()));
  }

  if (!ctx->config.timeout) {
    computedTimeLimit = static_cast<std::size_t>(std::ceil(testElapsed * kAutoTimeoutFactor)) + kAutoTimeoutPaddingSecs;
    Logger::info("Timeout: {} (auto)", Timestamper::format(computedTimeLimit));
    WorkspaceStatus status;
    status.originalTime = computedTimeLimit;
    ctx->workspace.saveStatus(status);
  }

  io::syncFiles(ctx->config.testResultDir, ctx->workspace.getOriginalResultsDir());

  if (fs::is_empty(ctx->workspace.getOriginalResultsDir())) {
    throw std::runtime_error(fmt::format("No test result files found in '{}' after running test command. See: {}",
                                         ctx->config.testResultDir.string(), testLog.string()));
  }

  Logger::info("Original test completed ({})", Timestamper::format(testElapsed));
  ctx->workspace.saveConfig(ctx->config);
  return true;
}

}  // namespace sentinel
