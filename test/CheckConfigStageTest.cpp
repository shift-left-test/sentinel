/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/CheckConfigStage.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class CheckConfigStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Logger::clearCache();
    mBase = fs::temp_directory_path() / "SENTINEL_CHECKCONFIG_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mLogger = Logger::getLogger("test");
    // Valid baseline config
    mConfig.buildCmd = "make";
    mConfig.testCmd = "make test";
    mConfig.testResultDir = mBase / "results";
    mConfig.force = true;  // skip interactive prompts
    mConfig.limit = 0u;
    mConfig.timeout = std::string("auto");
    mConfig.excludes = std::vector<std::string>{};
    mConfig.patterns = std::vector<std::string>{};
    mConfig.sourceDir = mBase;
  }
  void TearDown() override {
    fs::remove_all(mBase);
    Logger::clearCache();
  }

  Config mConfig;
  StatusLine mStatusLine;
  std::shared_ptr<Logger> mLogger;
  fs::path mBase;
};

TEST_F(CheckConfigStageTest, testPassesWithValidConfig) {
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_EQ(stage.handle(), 0);
}

TEST_F(CheckConfigStageTest, testThrowsWhenBuildCmdEmpty) {
  mConfig.buildCmd = "";
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_THROW(stage.handle(), InvalidArgumentException);
}

TEST_F(CheckConfigStageTest, testThrowsWhenTestCmdEmpty) {
  mConfig.testCmd = "";
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_THROW(stage.handle(), InvalidArgumentException);
}

TEST_F(CheckConfigStageTest, testThrowsWhenTestResultDirEmpty) {
  mConfig.testResultDir = fs::path("");
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_THROW(stage.handle(), InvalidArgumentException);
}

TEST_F(CheckConfigStageTest, testThrowsWhenThresholdOutOfRange) {
  mConfig.threshold = 150.0;
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_THROW(stage.handle(), InvalidArgumentException);
}

TEST_F(CheckConfigStageTest, testSkipsValidationWhenAlreadyComplete) {
  // Create run.done to simulate already-complete workspace
  fs::path ws = mBase / ".sentinel";
  fs::create_directories(ws);
  { std::ofstream f(ws / "run.done"); }
  mConfig.buildCmd = "";  // would normally throw
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_EQ(stage.handle(), 0);  // skipped, no throw
}

TEST_F(CheckConfigStageTest, testSkipsValidationWhenResuming) {
  // Create config.yaml (no run.done) to simulate resume
  fs::path ws = mBase / ".sentinel";
  fs::create_directories(ws);
  { std::ofstream f(ws / "config.yaml"); f << "version: 1"; }
  mConfig.buildCmd = "";  // would normally throw
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_EQ(stage.handle(), 0);
}

TEST_F(CheckConfigStageTest, testThrowsOnInvalidPartitionFormat) {
  mConfig.partition = std::string("bad");
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_THROW(stage.handle(), InvalidArgumentException);
}

TEST_F(CheckConfigStageTest, testThrowsWhenPartitionWithoutSeed) {
  mConfig.partition = std::string("1/4");
  // seed not set
  CheckConfigStage stage(mConfig, mStatusLine, mLogger, mBase / ".sentinel");
  EXPECT_THROW(stage.handle(), InvalidArgumentException);
}
}  // namespace sentinel
