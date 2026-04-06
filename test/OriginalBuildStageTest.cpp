/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <stdexcept>
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/PipelineContext.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/OriginalBuildStage.hpp"

namespace fs = std::filesystem;

using ::testing::HasSubstr;
using ::testing::ThrowsMessage;

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

    mWorkspace = std::make_shared<Workspace>(mWorkDir);

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

  PipelineContext makeCtx() {
    return {mConfig, mStatusLine, *mWorkspace};
  }

  fs::path mBase;
  fs::path mWorkDir;
  fs::path mCompileDbDir;
  Config mConfig;
  StatusLine mStatusLine;
  std::shared_ptr<Workspace> mWorkspace;
};

TEST_F(OriginalBuildStageTest, testShouldSkipWhenBuildLogExists) {
  fs::create_directories(mWorkspace->getOriginalDir());
  std::ofstream f(mWorkspace->getOriginalBuildLog());
  f << "build log";
  f.close();

  mConfig.buildCmd = "false";  // would fail if actually executed
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_NO_THROW(stage->run(&ctx));
}

TEST_F(OriginalBuildStageTest, testExecuteSuccessfulBuild) {
  fs::create_directories(mWorkspace->getOriginalDir());
  createCompileCommandsJson();

  mConfig.buildCmd = "true";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_NO_THROW(stage->run(&ctx));
  EXPECT_TRUE(fs::exists(mWorkspace->getOriginalBuildLog()));
}

TEST_F(OriginalBuildStageTest, testExecuteFailedBuildThrows) {
  fs::create_directories(mWorkspace->getOriginalDir());

  mConfig.buildCmd = "false";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_THROW(stage->run(&ctx), std::runtime_error);
}

TEST_F(OriginalBuildStageTest, testMissingCompileCommandsThrows) {
  fs::create_directories(mWorkspace->getOriginalDir());
  // compile_commands.json intentionally not created

  mConfig.buildCmd = "true";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_THROW(stage->run(&ctx), std::runtime_error);
}

TEST_F(OriginalBuildStageTest, testExecuteVerboseModeSuccessfulBuild) {
  fs::create_directories(mWorkspace->getOriginalDir());
  createCompileCommandsJson();

  mConfig.verbose = true;
  mConfig.buildCmd = "true";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_NO_THROW(stage->run(&ctx));
  EXPECT_TRUE(fs::exists(mWorkspace->getOriginalBuildLog()));
}

TEST_F(OriginalBuildStageTest, testExecuteVerboseModeBuildFailed) {
  fs::create_directories(mWorkspace->getOriginalDir());

  mConfig.verbose = true;
  mConfig.buildCmd = "false";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_THAT(
      [&]() { stage->run(&ctx); },
      ThrowsMessage<std::runtime_error>(
          HasSubstr("Original build failed")));
}

}  // namespace sentinel
