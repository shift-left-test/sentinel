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
#include <string>
#include "sentinel/Config.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/stages/InitStage.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class InitStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Logger::clearCache();
    mBase = fs::temp_directory_path() / "SENTINEL_INITSTAGE_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mOrigCwd = fs::current_path();
    fs::current_path(mBase);
    mLogger = Logger::getLogger("test");
  }
  void TearDown() override {
    fs::current_path(mOrigCwd);
    fs::remove_all(mBase);
    Logger::clearCache();
  }
  Config mConfig;
  StatusLine mStatusLine;
  std::shared_ptr<Logger> mLogger;
  fs::path mBase;
  fs::path mOrigCwd;
};

TEST_F(InitStageTest, testPassesThroughWhenInitNotSet) {
  mConfig.init = false;
  InitStage stage(mConfig, mStatusLine, mLogger);
  // Returns true (pass through) — handle() returns 0
  int code = stage.handle();
  EXPECT_EQ(code, 0);
  EXPECT_FALSE(fs::exists("sentinel.yaml"));
}

TEST_F(InitStageTest, testWritesSentinelYamlWhenInitSet) {
  mConfig.init = true;
  mConfig.force = true;
  InitStage stage(mConfig, mStatusLine, mLogger);
  int code = stage.handle();
  EXPECT_EQ(code, 0);
  EXPECT_TRUE(fs::exists("sentinel.yaml"));
}

TEST_F(InitStageTest, testDoesNotOverwriteWithoutForceWhenUserDeclines) {
  { std::ofstream f("sentinel.yaml"); f << "existing"; }
  mConfig.init = true;
  mConfig.force = false;

  // Redirect stdin to /dev/null to simulate non-interactive environment
  int savedStdin = dup(STDIN_FILENO);
  int devNull = open("/dev/null", O_RDONLY);
  dup2(devNull, STDIN_FILENO);
  close(devNull);

  InitStage stage(mConfig, mStatusLine, mLogger);
  EXPECT_NO_THROW(stage.handle());

  dup2(savedStdin, STDIN_FILENO);
  close(savedStdin);

  std::ifstream f("sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(f)), {});
  EXPECT_EQ(content, "existing");
}

TEST_F(InitStageTest, testOverwritesWithForce) {
  { std::ofstream f("sentinel.yaml"); f << "old content"; }
  mConfig.init = true;
  mConfig.force = true;
  InitStage stage(mConfig, mStatusLine, mLogger);
  stage.handle();
  std::ifstream f("sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(f)), {});
  EXPECT_NE(content, "old content");
  EXPECT_FALSE(content.empty());
}
}  // namespace sentinel
