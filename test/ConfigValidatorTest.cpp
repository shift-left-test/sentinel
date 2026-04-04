/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/ConfigValidator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "helper/TestTempDir.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class ConfigValidatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_CONFIGVALIDATOR_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    // Valid baseline config
    mConfig.buildCmd = "make";
    mConfig.testCmd = "make test";
    mConfig.testResultDir = mBase / "results";
    mConfig.limit = 10u;

    mConfig.patterns = {};
    mConfig.sourceDir = mBase;
  }
  void TearDown() override {
    fs::remove_all(mBase);
  }

  Config mConfig;
  fs::path mBase;
};

TEST_F(ConfigValidatorTest, testPassesWithValidConfig) {
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
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
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
  mConfig.threshold = 100.0;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testThrowsOnInvalidPartitionFormat) {
  mConfig.partition = std::string("bad");
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testWarningWhenPartitionWithoutSeed) {
  mConfig.partition = std::string("1/4");
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
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

TEST_F(ConfigValidatorTest, testWarningForTimeoutZero) {
  mConfig.timeout = static_cast<size_t>(0);
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForNegationPatternEndingWithSlash) {
  mConfig.patterns = {"!somedir/"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForNegationPatternWithoutWildcard) {
  mConfig.patterns = {"!relative/pattern.cpp"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForAbsolutePatternOutsideSourceDir) {
  mConfig.patterns = {"/tmp/outside/path.cpp"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testWarningForAbsolutePatternInsideSourceDir) {
  mConfig.patterns = {(mBase / "file.cpp").string()};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testMergeModeSkipsRequiredFieldValidation) {
  Config cfg = Config::withDefaults();
  cfg.mergeWorkspaces = {"/data/part1"};
  // buildCmd, testCmd, testResultDir are all empty — normally would throw
  EXPECT_NO_THROW(ConfigValidator::validate(cfg));
}

}  // namespace sentinel
