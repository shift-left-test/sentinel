/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <cmath>
#include <filesystem>  // NOLINT
#include <fstream>
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

// Serializes resolved config to YAML for workspace/config.yaml.
static std::string buildWorkspaceYaml(const Config& cfg) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "version" << YAML::Value << 1;
  out << YAML::Key << "source-dir" << YAML::Value << cfg.sourceDir.string();
  if (!cfg.outputDir.empty()) {
    out << YAML::Key << "output-dir" << YAML::Value << cfg.outputDir.string();
  }
  out << YAML::Key << "compiledb-dir" << YAML::Value << cfg.compileDbDir.string();
  out << YAML::Key << "scope" << YAML::Value << cfg.scope;
  out << YAML::Key << "extension" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : cfg.extensions) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "pattern" << YAML::Value << YAML::BeginSeq;
  for (const auto& p : cfg.patterns) out << p;
  out << YAML::EndSeq;
  out << YAML::Key << "build-command" << YAML::Value << cfg.buildCmd;
  out << YAML::Key << "test-command" << YAML::Value << cfg.testCmd;
  out << YAML::Key << "test-result-dir" << YAML::Value << cfg.testResultDir.string();
  out << YAML::Key << "test-result-ext" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : cfg.testResultExts) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "coverage" << YAML::Value << YAML::BeginSeq;
  for (const auto& c : cfg.coverageFiles) out << c.string();
  out << YAML::EndSeq;
  out << YAML::Key << "generator" << YAML::Value << cfg.generator;
  if (cfg.timeout) {
    out << YAML::Key << "timeout" << YAML::Value << *cfg.timeout;
  }
  out << YAML::Key << "kill-after" << YAML::Value << cfg.killAfter;
  out << YAML::Key << "operator" << YAML::Value << YAML::BeginSeq;
  for (const auto& op : cfg.operators) out << op;
  out << YAML::EndSeq;
  out << YAML::EndMap;
  return out.c_str();
}

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
  mWorkspace->saveConfig(buildWorkspaceYaml(mConfig));
  return true;
}

}  // namespace sentinel
