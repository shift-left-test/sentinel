/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <stdexcept>
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/OriginalBuildStage.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class OriginalBuildStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_ORIGBUILD_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);

    mWorkDir = mBase / "workspace";
    mCompileDbDir = mBase / "build";
    fs::create_directories(mWorkDir);
    fs::create_directories(mCompileDbDir);

    mConfig.workDir = mWorkDir;
    mConfig.compileDbDir = mCompileDbDir;
    mConfig.verbose = false;
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  void createCompileCommandsJson() {
    std::ofstream f(mCompileDbDir / "compile_commands.json");
    f << "[]";
  }

  fs::path mBase;
  fs::path mWorkDir;
  fs::path mCompileDbDir;
  Config mConfig;
  std::shared_ptr<StatusLine> mStatusLine = std::make_shared<StatusLine>();
};

TEST_F(OriginalBuildStageTest, testShouldSkipWhenBuildLogExists) {
  auto workspace = std::make_shared<Workspace>(mWorkDir);
  fs::create_directories(workspace->getOriginalDir());
  std::ofstream f(workspace->getOriginalBuildLog());
  f << "build log";
  f.close();

  mConfig.buildCmd = "false";  // would fail if actually executed
  OriginalBuildStage stage(mConfig, mStatusLine, workspace);

  EXPECT_NO_THROW(stage.run());
}

TEST_F(OriginalBuildStageTest, testExecuteSuccessfulBuild) {
  auto workspace = std::make_shared<Workspace>(mWorkDir);
  fs::create_directories(workspace->getOriginalDir());
  createCompileCommandsJson();

  mConfig.buildCmd = "true";
  OriginalBuildStage stage(mConfig, mStatusLine, workspace);

  EXPECT_NO_THROW(stage.run());
  EXPECT_TRUE(fs::exists(workspace->getOriginalBuildLog()));
}

TEST_F(OriginalBuildStageTest, testExecuteFailedBuildThrows) {
  auto workspace = std::make_shared<Workspace>(mWorkDir);
  fs::create_directories(workspace->getOriginalDir());

  mConfig.buildCmd = "false";
  OriginalBuildStage stage(mConfig, mStatusLine, workspace);

  EXPECT_THROW(stage.run(), std::runtime_error);
}

TEST_F(OriginalBuildStageTest, testMissingCompileCommandsThrows) {
  auto workspace = std::make_shared<Workspace>(mWorkDir);
  fs::create_directories(workspace->getOriginalDir());
  // compile_commands.json intentionally not created

  mConfig.buildCmd = "true";
  OriginalBuildStage stage(mConfig, mStatusLine, workspace);

  EXPECT_THROW(stage.run(), std::runtime_error);
}

}  // namespace sentinel
