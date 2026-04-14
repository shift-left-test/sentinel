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
#include <string>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "helper/ThrowMessageMatcher.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/PipelineContext.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/OriginalBuildStage.hpp"

namespace fs = std::filesystem;

using ::testing::AllOf;
using ::testing::HasSubstr;

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

TEST_F(OriginalBuildStageTest, testExecuteVerboseModeSuccessfulBuild) {
  fs::create_directories(mWorkspace->getOriginalDir());

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

  EXPECT_THROW_MESSAGE(
      stage->run(&ctx),
      std::runtime_error, HasSubstr("Original build failed"));
}

TEST_F(OriginalBuildStageTest, testBuildLogContainsOutput) {
  fs::create_directories(mWorkspace->getOriginalDir());

  mConfig.buildCmd = "echo 'build output here'";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_NO_THROW(stage->run(&ctx));
  EXPECT_TRUE(fs::exists(mWorkspace->getOriginalBuildLog()));

  std::string content = testutil::readFile(mWorkspace->getOriginalBuildLog());
  EXPECT_NE(content.find("build output here"), std::string::npos)
      << "Build log should contain the build command output";
}

TEST_F(OriginalBuildStageTest, testFailedBuildMessageContainsBuildCommand) {
  fs::create_directories(mWorkspace->getOriginalDir());

  mConfig.buildCmd = "echo 'cmake error output' && false";
  auto stage = std::make_shared<OriginalBuildStage>();
  auto ctx = makeCtx();

  EXPECT_THROW_MESSAGE(
      stage->run(&ctx),
      std::runtime_error,
      AllOf(HasSubstr("Original build failed"),
            HasSubstr("Build command: echo 'cmake error output' && false")));
}

}  // namespace sentinel
