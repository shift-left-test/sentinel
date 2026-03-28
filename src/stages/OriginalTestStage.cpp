/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <algorithm>
#include <cmath>
#include <filesystem>  // NOLINT
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/stages/OriginalTestStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kAutoTimeoutPaddingSecs = 5;

OriginalTestStage::OriginalTestStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                                     std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool OriginalTestStage::shouldSkip() const {
  return fs::exists(mWorkspace->getOriginalTestLog()) && mWorkspace->hasPreviousRun();
}

StatusLine::Phase OriginalTestStage::getPhase() const {
  return StatusLine::Phase::TEST_ORIG;
}

bool OriginalTestStage::execute() {
  Logger::info("Running original test...");
  fs::path testLog = mWorkspace->getOriginalTestLog();
  Logger::verbose("Test command: {}", mConfig.testCmd);
  Logger::verbose("Test log: {}", testLog);
  Logger::verbose("Test result dir: {} (ext: {})",
                  mConfig.testResultDir.string(),
                  fmt::join(mConfig.testResultExts, ", "));

  const std::size_t killAfterSecs = mConfig.killAfter;
  std::size_t computedTimeLimit = 0;
  if (mConfig.timeout.has_value()) {
    computedTimeLimit = *mConfig.timeout;
    Logger::info("Timeout: {}s, kill-after: {}s", computedTimeLimit, killAfterSecs);
  }

  Timestamper testTimer;
  Subprocess testProc(mConfig.testCmd, computedTimeLimit, killAfterSecs, testLog.string(),
                      !isVerbose());
  testProc.execute();
  const double testElapsed = testTimer.toDouble();

  if (!mConfig.timeout) {
    computedTimeLimit = static_cast<std::size_t>(std::ceil(testElapsed * 2.0)) + kAutoTimeoutPaddingSecs;
    Logger::info("Timeout: {}s (auto), kill-after: {}s", computedTimeLimit, killAfterSecs);
    WorkspaceStatus status;
    status.originalTime = computedTimeLimit;
    mWorkspace->saveStatus(status);
  }

  io::syncFiles(mConfig.testResultDir, mWorkspace->getOriginalResultsDir(), mConfig.testResultExts);

  if (fs::is_empty(mWorkspace->getOriginalResultsDir())) {
    throw std::runtime_error(fmt::format("No test result files found in '{}' after running test command. See: {}",
                                         mConfig.testResultDir.string(), testLog.string()));
  }

  Logger::info("Original test completed ({})", Timestamper::format(testElapsed));
  mWorkspace->saveConfig(mConfig);
  return true;
}

}  // namespace sentinel
