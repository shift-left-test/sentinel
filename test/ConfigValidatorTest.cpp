/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/ConfigValidator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class ConfigValidatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = fs::temp_directory_path() / "SENTINEL_CONFIGVALIDATOR_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    // Valid baseline config
    mConfig.buildCmd = "make";
    mConfig.testCmd = "make test";
    mConfig.testResultDir = mBase / "results";
    mConfig.force = true;  // skip interactive prompts
    mConfig.limit = 10u;
    mConfig.timeout = std::string("auto");
    mConfig.excludes = std::vector<std::string>{};
    mConfig.patterns = std::vector<std::string>{};
    mConfig.sourceDir = mBase;
  }
  void TearDown() override { fs::remove_all(mBase); }

  Config mConfig;
  fs::path mBase;
};

TEST_F(ConfigValidatorTest, testPassesWithValidConfig) {
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testThrowsWhenBuildCmdEmpty) {
  mConfig.buildCmd = "";
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenTestCmdEmpty) {
  mConfig.testCmd = "";
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenTestResultDirEmpty) {
  mConfig.testResultDir = fs::path("");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenThresholdOutOfRange) {
  mConfig.threshold = 150.0;
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThresholdBelowZeroThrows) {
  mConfig.threshold = -1.0;
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThresholdAtBoundaryIsValid) {
  mConfig.threshold = 0.0;
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
  mConfig.threshold = 100.0;
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testThrowsOnInvalidPartitionFormat) {
  mConfig.partition = std::string("bad");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionWithoutSeed) {
  mConfig.partition = std::string("1/4");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionSlashAtStart) {
  mConfig.partition = std::string("/4");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionSlashAtEnd) {
  mConfig.partition = std::string("1/");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionHasNonNumericParts) {
  mConfig.partition = std::string("abc/def");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionCountIsZero) {
  mConfig.partition = std::string("1/0");
  mConfig.seed = 42u;
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionIndexIsZero) {
  mConfig.partition = std::string("0/4");
  mConfig.seed = 42u;
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsWhenPartitionIndexExceedsCount) {
  mConfig.partition = std::string("5/4");
  mConfig.seed = 42u;
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testWarningForTimeoutZeroWithForce) {
  mConfig.timeout = std::string("0");
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForExcludeEndingWithSlash) {
  mConfig.excludes = std::vector<std::string>{"somedir/"};
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForRelativeExcludeWithoutWildcard) {
  mConfig.excludes = std::vector<std::string>{"relative/pattern.cpp"};
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForAbsolutePatternOutsideSourceDir) {
  mConfig.patterns = std::vector<std::string>{"/tmp/outside/path.cpp"};
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForAbsolutePatternInsideSourceDir) {
  mConfig.patterns = std::vector<std::string>{(mBase / "file.cpp").string()};
  EXPECT_TRUE(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningsAbortWhenNotForced) {
  // Trigger the limit warning by setting limit=0u when force=false
  mConfig.limit = 0u;
  mConfig.force = false;
  int savedStdin = dup(STDIN_FILENO);
  int devNull = open("/dev/null", O_RDONLY);
  dup2(devNull, STDIN_FILENO);
  close(devNull);

  // validate() returns false when user declines (stdin=/dev/null → confirm returns false)
  EXPECT_FALSE(ConfigValidator::validate(mConfig));

  dup2(savedStdin, STDIN_FILENO);
  close(savedStdin);
}
}  // namespace sentinel
