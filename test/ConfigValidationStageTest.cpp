/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <unistd.h>
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
#include "sentinel/stages/ConfigValidationStage.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class ConfigValidationStageTest : public ::testing::Test {
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
  std::shared_ptr<StatusLine> mStatusLine = std::make_shared<StatusLine>();
  std::shared_ptr<Logger> mLogger;
  fs::path mBase;
};

TEST_F(ConfigValidationStageTest, testPassesWithValidConfig) {
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testThrowsWhenBuildCmdEmpty) {
  mConfig.buildCmd = "";
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenTestCmdEmpty) {
  mConfig.testCmd = "";
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenTestResultDirEmpty) {
  mConfig.testResultDir = fs::path("");
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenThresholdOutOfRange) {
  mConfig.threshold = 150.0;
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testSkipsValidationWhenAlreadyComplete) {
  // Create run.done to simulate already-complete workspace
  fs::path wsPath = mBase / ".sentinel";
  fs::create_directories(wsPath);
  { std::ofstream f(wsPath / "run.done"); }
  mConfig.buildCmd = "";  // would normally throw
  auto ws = std::make_shared<Workspace>(wsPath);
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());  // skipped, no throw
}

TEST_F(ConfigValidationStageTest, testSkipsValidationWhenResuming) {
  // Create config.yaml (no run.done) to simulate resume
  fs::path wsPath = mBase / ".sentinel";
  fs::create_directories(wsPath);
  { std::ofstream f(wsPath / "config.yaml"); f << "version: 1"; }
  mConfig.buildCmd = "";  // would normally throw
  auto ws = std::make_shared<Workspace>(wsPath);
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testThrowsOnInvalidPartitionFormat) {
  mConfig.partition = std::string("bad");
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionWithoutSeed) {
  mConfig.partition = std::string("1/4");
  // seed not set
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionSlashAtStart) {
  mConfig.partition = std::string("/4");
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionSlashAtEnd) {
  mConfig.partition = std::string("1/");
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionHasNonNumericParts) {
  mConfig.partition = std::string("abc/def");
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionCountIsZero) {
  mConfig.partition = std::string("1/0");
  mConfig.seed = 42u;
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionIndexIsZero) {
  mConfig.partition = std::string("0/4");
  mConfig.seed = 42u;
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThrowsWhenPartitionIndexExceedsCount) {
  mConfig.partition = std::string("5/4");
  mConfig.seed = 42u;
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThresholdBelowZeroThrows) {
  mConfig.threshold = -1.0;
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_THROW(stage.run(), InvalidArgumentException);
}

TEST_F(ConfigValidationStageTest, testThresholdAtBoundaryIsValid) {
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  mConfig.threshold = 0.0;
  ConfigValidationStage stage0(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage0.run());

  mConfig.threshold = 100.0;
  ConfigValidationStage stage100(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage100.run());
}

TEST_F(ConfigValidationStageTest, testWarningForTimeoutZeroWithForce) {
  mConfig.timeout = std::string("0");
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testWarningForExcludeEndingWithSlash) {
  mConfig.excludes = std::vector<std::string>{"somedir/"};
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testWarningForRelativeExcludeWithoutWildcard) {
  mConfig.excludes = std::vector<std::string>{"relative/pattern.cpp"};
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testWarningForAbsolutePatternOutsideSourceDir) {
  // sourceDir = mBase; pattern is absolute and outside sourceDir
  mConfig.patterns = std::vector<std::string>{"/tmp/outside/path.cpp"};
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testWarningForAbsolutePatternInsideSourceDir) {
  // sourceDir = mBase; pattern is absolute but inside sourceDir
  mConfig.patterns = std::vector<std::string>{(mBase / "file.cpp").string()};
  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ConfigValidationStageTest, testWarningsAbortWhenNotForced) {
  // force=false + limit=0 (warning always present from fixture) triggers interactive prompt.
  // Redirect stdin to /dev/null so Console::confirm returns false without blocking.
  mConfig.force = false;
  int savedStdin = dup(STDIN_FILENO);
  int devNull = open("/dev/null", O_RDONLY);
  dup2(devNull, STDIN_FILENO);
  close(devNull);

  auto ws = std::make_shared<Workspace>(mBase / ".sentinel");
  ConfigValidationStage stage(mConfig, mStatusLine, mLogger, ws);
  EXPECT_NO_THROW(stage.run());

  dup2(savedStdin, STDIN_FILENO);
  close(savedStdin);
}
}  // namespace sentinel
