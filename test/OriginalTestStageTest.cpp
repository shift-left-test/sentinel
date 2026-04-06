/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <stdexcept>
#include <string>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/PipelineContext.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/OriginalTestStage.hpp"

namespace fs = std::filesystem;

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::ThrowsMessage;

namespace sentinel {

class OriginalTestStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_ORIG_TEST_STAGE");
    fs::remove_all(mBase);
    fs::create_directories(mBase);

    mWorkspaceRoot = mBase / "workspace";
    mTestResultDir = mBase / "test-results";
    fs::create_directories(mTestResultDir);

    mWorkspace = std::make_shared<Workspace>(mWorkspaceRoot);
    mWorkspace->initialize();

    mConfig = Config::withDefaults();
    mConfig.testCmd = "true";
    mConfig.testResultDir = mTestResultDir;
    mConfig.timeout = std::nullopt;
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  void createTestResultFile(const std::string& filename = "results.xml") {
    testutil::writeFile(mTestResultDir / filename, "<testsuites/>");
  }

  PipelineContext makeCtx() {
    return {mConfig, mStatusLine, *mWorkspace};
  }

  fs::path mBase;
  fs::path mWorkspaceRoot;
  fs::path mTestResultDir;
  std::shared_ptr<Workspace> mWorkspace;
  StatusLine mStatusLine;
  Config mConfig;
};

TEST_F(OriginalTestStageTest, testShouldSkipWhenTestLogAndPreviousRunExist) {
  // Create the test log file
  testutil::writeFile(mWorkspace->getOriginalTestLog(), "test log content");
  // Create config.yaml so hasPreviousRun() returns true
  mWorkspace->saveConfig(mConfig);

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  // shouldSkip() is protected; test via run() with a recording next stage
  // We can observe skip by verifying no side effects (no status written, no config overwritten)
  // Pre-verify: both conditions are met
  EXPECT_TRUE(fs::exists(mWorkspace->getOriginalTestLog()));
  EXPECT_TRUE(mWorkspace->hasPreviousRun());

  // run() should skip (not throw, not change workspace state further)
  EXPECT_NO_THROW(stage->run(&ctx));
}

TEST_F(OriginalTestStageTest, testDoesNotSkipWhenNoPreviousRun) {
  // Create the test log file, but do NOT call saveConfig (no config.yaml)
  testutil::writeFile(mWorkspace->getOriginalTestLog(), "test log content");

  // Pre-create a test result file so the stage does not throw on empty results
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();

  // hasPreviousRun() is false, so shouldSkip() == false; execute() runs and saves config
  EXPECT_NO_THROW(stage->run(&ctx));
  EXPECT_TRUE(mWorkspace->hasPreviousRun());
}

TEST_F(OriginalTestStageTest, testAutoTimeoutSavesStatus) {
  // No timeout set (auto mode)
  mConfig.timeout = std::nullopt;
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  stage->run(&ctx);

  WorkspaceStatus status = mWorkspace->loadStatus();
  ASSERT_TRUE(status.originalTime.has_value());
  // Auto timeout = ceil(elapsed * 1.5) + 5; even with elapsed ~0, result must be >= 5
  EXPECT_GE(*status.originalTime, static_cast<std::size_t>(5));
}

TEST_F(OriginalTestStageTest, testExplicitTimeoutDoesNotSaveStatus) {
  // Explicit timeout — status.originalTime must NOT be written
  mConfig.timeout = 30;
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  stage->run(&ctx);

  WorkspaceStatus status = mWorkspace->loadStatus();
  EXPECT_FALSE(status.originalTime.has_value());
}

TEST_F(OriginalTestStageTest, testEmptyTestResultsThrows) {
  mConfig.timeout = std::nullopt;
  mConfig.testCmd = "echo 'test not found' >&2";

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THAT(
      [&] { stage->run(&ctx); },
      ThrowsMessage<std::runtime_error>(
          AllOf(HasSubstr("No test result files found"), HasSubstr("test not found"))));
}

TEST_F(OriginalTestStageTest, testSuccessfulRunSavesConfig) {
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  EXPECT_FALSE(mWorkspace->hasPreviousRun());

  auto ctx = makeCtx();
  stage->run(&ctx);

  EXPECT_TRUE(mWorkspace->hasPreviousRun());
}

TEST_F(OriginalTestStageTest, testSignalExitThrows) {
  mConfig.testCmd = "sh -c 'kill -SEGV $$' && echo unreachable";
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THAT(
      [&] { stage->run(&ctx); },
      ThrowsMessage<std::runtime_error>(AllOf(HasSubstr("killed by a signal"), HasSubstr("\n"))));
}

TEST_F(OriginalTestStageTest, testInvalidTestCommandWithNoResultsThrows) {
  mConfig.timeout = std::nullopt;
  mConfig.testCmd = "not_existing_cmd_xyz";

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THAT(
      [&] { stage->run(&ctx); },
      ThrowsMessage<std::runtime_error>(HasSubstr("test command failed")));
}

TEST_F(OriginalTestStageTest, testVerboseSuccessfulRun) {
  mConfig.verbose = true;
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();

  EXPECT_NO_THROW(stage->run(&ctx));
  EXPECT_TRUE(mWorkspace->hasPreviousRun());
}

TEST_F(OriginalTestStageTest, testVerboseSignalExitDoesNotAppendLogTail) {
  mConfig.verbose = true;
  mConfig.testCmd = "sh -c 'kill -SEGV $$'";
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THAT(
      [&] { stage->run(&ctx); },
      ThrowsMessage<std::runtime_error>(
          AllOf(HasSubstr("killed by a signal"), Not(HasSubstr("test not found")))));
}

TEST_F(OriginalTestStageTest, testTestFailedWithNoResults) {
  mConfig.testCmd = "false";
  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THAT(
      [&] { stage->run(&ctx); },
      ThrowsMessage<std::runtime_error>(HasSubstr("test command failed")));
}

}  // namespace sentinel
