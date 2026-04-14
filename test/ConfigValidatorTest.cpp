/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/ConfigValidator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "helper/TestTempDir.hpp"
#include "helper/ThrowMessageMatcher.hpp"

namespace sentinel {
namespace fs = std::filesystem;

using ::testing::HasSubstr;

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

TEST_F(ConfigValidatorTest, testThrowsOnInvalidOperator) {
  mConfig.operators = {"AAA"};
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testThrowsOnMixedValidAndInvalidOperators) {
  mConfig.operators = {"AOR", "XXX"};
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testAcceptsValidOperators) {
  mConfig.operators = {"AOR", "BOR", "LCR", "ROR", "SDL", "SOR", "UOI"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testAcceptsValidOperatorsCaseInsensitive) {
  mConfig.operators = {"aor", "Bor"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testEmptyOperatorsIsValid) {
  mConfig.operators = {};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testMergeModeSkipsRequiredFieldValidation) {
  Config cfg = Config::withDefaults();
  cfg.mergeWorkspaces = {"/data/part1"};
  // buildCmd, testCmd, testResultDir are all empty — normally would throw
  EXPECT_NO_THROW(ConfigValidator::validate(cfg));
}

TEST_F(ConfigValidatorTest, testFromEmptyStringThrows) {
  mConfig.from = "";
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testFromDoubleDotRangeThrows) {
  mConfig.from = "HEAD..main";
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testFromTripleDotRangeThrows) {
  mConfig.from = "HEAD...main";
  EXPECT_THROW(ConfigValidator::validate(mConfig), InvalidArgumentException);
}

TEST_F(ConfigValidatorTest, testFromValidRevisionPasses) {
  mConfig.from = "HEAD~1";
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testFromNulloptPasses) {
  mConfig.from = std::nullopt;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testUncommittedAloneIsValid) {
  mConfig.uncommitted = true;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testValidPartitionWithSeedPasses) {
  mConfig.partition = std::string("2/4");
  mConfig.seed = 42u;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testThresholdJustInsideBoundaryPasses) {
  mConfig.threshold = 0.001;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
  mConfig.threshold = 99.999;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testNonZeroTimeoutIsValid) {
  mConfig.timeout = 60u;
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testEmptyPatternIsIgnored) {
  mConfig.patterns = {""};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testNonNegationPatternEndingWithSlashNoWarning) {
  mConfig.patterns = {"somedir/"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testRelativePatternIsValid) {
  mConfig.patterns = {"src/**/*.cpp"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testNegationPatternWithoutSlashNoSlashWarning) {
  mConfig.patterns = {"!src/**/*.cpp"};
  EXPECT_NO_THROW(ConfigValidator::validate(mConfig));
}

TEST_F(ConfigValidatorTest, testThrowsWhenSourceDirDoesNotExist) {
  mConfig.sourceDir = mBase / "nonexistent";
  EXPECT_THROW_MESSAGE(
      ConfigValidator::validate(mConfig),
      InvalidArgumentException,
      HasSubstr("--source-dir"));
}

TEST_F(ConfigValidatorTest, testThrowsWhenSourceDirIsNotADirectory) {
  fs::path filePath = mBase / "regular-file";
  { std::ofstream ofs(filePath); }
  mConfig.sourceDir = filePath;
  EXPECT_THROW_MESSAGE(
      ConfigValidator::validate(mConfig),
      InvalidArgumentException,
      HasSubstr("--source-dir"));
}

TEST_F(ConfigValidatorTest, testThrowsWhenSourceDirIsNotReadable) {
  fs::path noReadDir = mBase / "noperm";
  fs::create_directory(noReadDir);
  fs::permissions(noReadDir, fs::perms::none);
  mConfig.sourceDir = noReadDir;
  EXPECT_THROW_MESSAGE(
      ConfigValidator::validate(mConfig),
      InvalidArgumentException,
      HasSubstr("--source-dir"));
  fs::permissions(noReadDir, fs::perms::all);
}

}  // namespace sentinel
