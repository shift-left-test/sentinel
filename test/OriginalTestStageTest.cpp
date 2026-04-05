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
#include "sentinel/PipelineContext.hpp"
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
  // testResultDir exists but has no .xml files — stage must throw
  mConfig.timeout = std::nullopt;
  // Do NOT call createTestResultFile()

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THROW(stage->run(&ctx), std::runtime_error);
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
  // When the test command exits with a signal-like code (128+N via sh -c),
  // the stage must throw instead of saving corrupted baseline results.
  mConfig.testCmd = "sh -c 'kill -SEGV $$' && echo unreachable";
  createTestResultFile();

  auto stage = std::make_shared<OriginalTestStage>();
  auto ctx = makeCtx();
  EXPECT_THROW(stage->run(&ctx), std::runtime_error);
}

}  // namespace sentinel
