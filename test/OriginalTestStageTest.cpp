/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <stdexcept>
#include <string>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/OriginalTestStage.hpp"

namespace fs = std::filesystem;

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

    mStatusLine = std::make_shared<StatusLine>();

    mConfig = Config::withDefaults();
    mConfig.testCmd = "true";
    mConfig.testResultDir = mTestResultDir;
    mConfig.testResultExts = {"xml", "XML"};
    mConfig.killAfter = 60;
    mConfig.timeout = std::nullopt;
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  void createTestResultFile(const std::string& filename = "results.xml") {
    testutil::writeFile(mTestResultDir / filename, "<testsuites/>");
  }

  fs::path mBase;
  fs::path mWorkspaceRoot;
  fs::path mTestResultDir;
  std::shared_ptr<Workspace> mWorkspace;
  std::shared_ptr<StatusLine> mStatusLine;
  Config mConfig;
};

TEST_F(OriginalTestStageTest, testShouldSkipWhenTestLogAndPreviousRunExist) {
  // Create the test log file
  testutil::writeFile(mWorkspace->getOriginalTestLog(), "test log content");
  // Create config.yaml so hasPreviousRun() returns true
  mWorkspace->saveConfig(mConfig);

  OriginalTestStage stage(mConfig, mStatusLine, mWorkspace);
  // shouldSkip() is protected; test via run() with a recording next stage
  // We can observe skip by verifying no side effects (no status written, no config overwritten)
  // Pre-verify: both conditions are met
  EXPECT_TRUE(fs::exists(mWorkspace->getOriginalTestLog()));
  EXPECT_TRUE(mWorkspace->hasPreviousRun());

  // run() should skip (not throw, not change workspace state further)
  EXPECT_NO_THROW(stage.run());
}

TEST_F(OriginalTestStageTest, testDoesNotSkipWhenNoPreviousRun) {
  // Create the test log file, but do NOT call saveConfig (no config.yaml)
  testutil::writeFile(mWorkspace->getOriginalTestLog(), "test log content");

  // Pre-create a test result file so the stage does not throw on empty results
  createTestResultFile();

  OriginalTestStage stage(mConfig, mStatusLine, mWorkspace);

  // hasPreviousRun() is false, so shouldSkip() == false; execute() runs and saves config
  EXPECT_NO_THROW(stage.run());
  EXPECT_TRUE(mWorkspace->hasPreviousRun());
}

TEST_F(OriginalTestStageTest, testAutoTimeoutSavesStatus) {
  // No timeout set (auto mode)
  mConfig.timeout = std::nullopt;
  createTestResultFile();

  OriginalTestStage stage(mConfig, mStatusLine, mWorkspace);
  stage.run();

  WorkspaceStatus status = mWorkspace->loadStatus();
  ASSERT_TRUE(status.originalTime.has_value());
  // Auto timeout = ceil(elapsed * 2.0) + 5; even with elapsed ~0, result must be >= 5
  EXPECT_GE(*status.originalTime, static_cast<std::size_t>(5));
}

TEST_F(OriginalTestStageTest, testExplicitTimeoutDoesNotSaveStatus) {
  // Explicit timeout — status.originalTime must NOT be written
  mConfig.timeout = 30;
  createTestResultFile();

  OriginalTestStage stage(mConfig, mStatusLine, mWorkspace);
  stage.run();

  WorkspaceStatus status = mWorkspace->loadStatus();
  EXPECT_FALSE(status.originalTime.has_value());
}

TEST_F(OriginalTestStageTest, testEmptyTestResultsThrows) {
  // testResultDir exists but has no .xml files — stage must throw
  mConfig.timeout = std::nullopt;
  // Do NOT call createTestResultFile()

  OriginalTestStage stage(mConfig, mStatusLine, mWorkspace);
  EXPECT_THROW(stage.run(), std::runtime_error);
}

TEST_F(OriginalTestStageTest, testSuccessfulRunSavesConfig) {
  createTestResultFile();

  OriginalTestStage stage(mConfig, mStatusLine, mWorkspace);
  EXPECT_FALSE(mWorkspace->hasPreviousRun());

  stage.run();

  EXPECT_TRUE(mWorkspace->hasPreviousRun());
}

}  // namespace sentinel
