/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/SentinelConfig.hpp"

namespace sentinel {

class SentinelConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    namespace fs = std::filesystem;
    mTmpDir = fs::temp_directory_path() / "SENTINEL_CONFIG_TEST";
    fs::remove_all(mTmpDir);
    fs::create_directories(mTmpDir);
  }

  void TearDown() override {
    std::filesystem::remove_all(mTmpDir);
  }

  void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream out((mTmpDir / filename).string());
    out << content;
    out.close();
  }

  std::string configPath(const std::string& filename) {
    return (mTmpDir / filename).string();
  }

  std::filesystem::path mTmpDir;
};

TEST_F(SentinelConfigTest, testLoadCompleteConfig) {
  writeFile("sentinel.yaml", R"(
source-dir: ./src
workspace: ./work
output-dir: ./out
compiledb-dir: ./build
scope: commit
extension:
  - cpp
  - cxx
pattern:
  - src/**
exclude:
  - test/**
limit: 5
build-command: make
test-command: ctest
test-report-dir: ./results
test-report-extension:
  - xml
coverage:
  - coverage.info
generator: random
timeout: 30
kill-after: 10
seed: 42
operator:
  - AOR
  - ROR
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));

  ASSERT_TRUE(cfg.sourceDir.has_value());
  EXPECT_EQ((mTmpDir / "src").string(), *cfg.sourceDir);

  ASSERT_TRUE(cfg.workDir.has_value());
  EXPECT_EQ((mTmpDir / "work").string(), *cfg.workDir);

  ASSERT_TRUE(cfg.outputDir.has_value());
  EXPECT_EQ((mTmpDir / "out").string(), *cfg.outputDir);

  ASSERT_TRUE(cfg.compileDbDir.has_value());
  EXPECT_EQ((mTmpDir / "build").string(), *cfg.compileDbDir);

  ASSERT_TRUE(cfg.scope.has_value());
  EXPECT_EQ("commit", *cfg.scope);

  ASSERT_TRUE(cfg.extensions.has_value());
  EXPECT_EQ(std::vector<std::string>({"cpp", "cxx"}), *cfg.extensions);

  ASSERT_TRUE(cfg.patterns.has_value());
  EXPECT_EQ(std::vector<std::string>({"src/**"}), *cfg.patterns);

  ASSERT_TRUE(cfg.excludes.has_value());
  EXPECT_EQ(std::vector<std::string>({"test/**"}), *cfg.excludes);

  ASSERT_TRUE(cfg.limit.has_value());
  EXPECT_EQ(5u, *cfg.limit);

  ASSERT_TRUE(cfg.buildCmd.has_value());
  EXPECT_EQ("make", *cfg.buildCmd);

  ASSERT_TRUE(cfg.testCmd.has_value());
  EXPECT_EQ("ctest", *cfg.testCmd);

  ASSERT_TRUE(cfg.testResultDir.has_value());
  EXPECT_EQ((mTmpDir / "results").string(), *cfg.testResultDir);

  ASSERT_TRUE(cfg.testResultFileExts.has_value());
  EXPECT_EQ(std::vector<std::string>({"xml"}), *cfg.testResultFileExts);

  ASSERT_TRUE(cfg.coverageFiles.has_value());
  EXPECT_EQ(std::vector<std::string>({(mTmpDir / "coverage.info").string()}), *cfg.coverageFiles);

  ASSERT_TRUE(cfg.generator.has_value());
  EXPECT_EQ("random", *cfg.generator);

  ASSERT_TRUE(cfg.timeLimit.has_value());
  EXPECT_EQ("30", *cfg.timeLimit);

  ASSERT_TRUE(cfg.killAfter.has_value());
  EXPECT_EQ("10", *cfg.killAfter);

  ASSERT_TRUE(cfg.seed.has_value());
  EXPECT_EQ(42u, *cfg.seed);

  ASSERT_TRUE(cfg.operators.has_value());
  EXPECT_EQ(std::vector<std::string>({"AOR", "ROR"}), *cfg.operators);

  EXPECT_FALSE(cfg.threshold.has_value());
}

TEST_F(SentinelConfigTest, testLoadPartialConfig) {
  writeFile("sentinel.yaml", R"(
build-command: make all
test-command: ctest --verbose
test-report-dir: ./test-results
limit: 20
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));

  ASSERT_TRUE(cfg.buildCmd.has_value());
  EXPECT_EQ("make all", *cfg.buildCmd);

  ASSERT_TRUE(cfg.testCmd.has_value());
  EXPECT_EQ("ctest --verbose", *cfg.testCmd);

  ASSERT_TRUE(cfg.testResultDir.has_value());
  EXPECT_EQ((mTmpDir / "test-results").string(), *cfg.testResultDir);

  ASSERT_TRUE(cfg.limit.has_value());
  EXPECT_EQ(20u, *cfg.limit);

  // Unspecified fields must remain empty optionals
  EXPECT_FALSE(cfg.sourceDir.has_value());
  EXPECT_FALSE(cfg.workDir.has_value());
  EXPECT_FALSE(cfg.outputDir.has_value());
  EXPECT_FALSE(cfg.compileDbDir.has_value());
  EXPECT_FALSE(cfg.scope.has_value());
  EXPECT_FALSE(cfg.extensions.has_value());
  EXPECT_FALSE(cfg.patterns.has_value());
  EXPECT_FALSE(cfg.excludes.has_value());
  EXPECT_FALSE(cfg.testResultFileExts.has_value());
  EXPECT_FALSE(cfg.coverageFiles.has_value());
  EXPECT_FALSE(cfg.generator.has_value());
  EXPECT_FALSE(cfg.timeLimit.has_value());
  EXPECT_FALSE(cfg.killAfter.has_value());
  EXPECT_FALSE(cfg.seed.has_value());
  EXPECT_FALSE(cfg.operators.has_value());
  EXPECT_FALSE(cfg.threshold.has_value());
}

TEST_F(SentinelConfigTest, testLoadEmptyConfig) {
  writeFile("empty.yaml", "");

  // Empty file is a null YAML node, not a mapping — should throw
  EXPECT_THROW(SentinelConfig::loadFromFile(configPath("empty.yaml")), std::runtime_error);
}

TEST_F(SentinelConfigTest, testLoadInvalidYamlSyntax) {
  writeFile("bad.yaml", "key: [unclosed");

  EXPECT_THROW(SentinelConfig::loadFromFile(configPath("bad.yaml")), std::runtime_error);
}

TEST_F(SentinelConfigTest, testLoadWrongTypeForLimit) {
  writeFile("sentinel.yaml", R"(
limit: not_a_number
)");

  EXPECT_THROW(SentinelConfig::loadFromFile(configPath("sentinel.yaml")), std::runtime_error);
}

TEST_F(SentinelConfigTest, testLoadExtensionNotAList) {
  writeFile("sentinel.yaml", R"(
extension: cpp
)");

  EXPECT_THROW(SentinelConfig::loadFromFile(configPath("sentinel.yaml")), std::runtime_error);
}

TEST_F(SentinelConfigTest, testUnknownKeysAreIgnored) {
  writeFile("sentinel.yaml", R"(
unknown-key: some-value
another-unknown: 42
build-command: make
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));
  ASSERT_TRUE(cfg.buildCmd.has_value());
  EXPECT_EQ("make", *cfg.buildCmd);
}

TEST_F(SentinelConfigTest, testTimeoutAsStringAuto) {
  writeFile("sentinel.yaml", R"(
timeout: auto
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));
  ASSERT_TRUE(cfg.timeLimit.has_value());
  EXPECT_EQ("auto", *cfg.timeLimit);
}

TEST_F(SentinelConfigTest, testTimeoutAsIntegerIsConvertedToString) {
  writeFile("sentinel.yaml", R"(
timeout: 60
kill-after: 15
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));
  ASSERT_TRUE(cfg.timeLimit.has_value());
  EXPECT_EQ("60", *cfg.timeLimit);
  ASSERT_TRUE(cfg.killAfter.has_value());
  EXPECT_EQ("15", *cfg.killAfter);
}

TEST_F(SentinelConfigTest, testMultipleOperators) {
  writeFile("sentinel.yaml", R"(
operator:
  - AOR
  - BOR
  - LCR
  - ROR
  - SDL
  - SOR
  - UOI
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));
  ASSERT_TRUE(cfg.operators.has_value());
  std::vector<std::string> expected = {"AOR", "BOR", "LCR", "ROR", "SDL", "SOR", "UOI"};
  EXPECT_EQ(expected, *cfg.operators);
}

TEST_F(SentinelConfigTest, testThresholdParsed) {
  writeFile("sentinel.yaml", R"(
build-command: make
threshold: 80.0
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(80.0, *cfg.threshold);
}

TEST_F(SentinelConfigTest, testThresholdInteger) {
  writeFile("sentinel.yaml", R"(
build-command: make
threshold: 75
)");

  auto cfg = SentinelConfig::loadFromFile(configPath("sentinel.yaml"));
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(75.0, *cfg.threshold);
}

TEST_F(SentinelConfigTest, testFileNotFoundThrows) {
  EXPECT_THROW(SentinelConfig::loadFromFile(configPath("nonexistent.yaml")), std::runtime_error);
}

}  // namespace sentinel
