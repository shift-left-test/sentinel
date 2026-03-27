/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <stdexcept>
#include <string>
#include <vector>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/YamlConfigParser.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mTmpDir = testTempDir("SENTINEL_CONFIG_TEST");
    fs::remove_all(mTmpDir);
    fs::create_directories(mTmpDir);
  }

  void TearDown() override {
    fs::remove_all(mTmpDir);
  }

  void writeFile(const std::string& filename, const std::string& content) {
    testutil::writeFile(mTmpDir / filename, content);
  }

  fs::path configPath(const std::string& filename) {
    return mTmpDir / filename;
  }

  fs::path mTmpDir;
};

TEST_F(ConfigTest, testWithDefaultsSetsAbsolutePaths) {
  auto cfg = Config::withDefaults();
  EXPECT_TRUE(cfg.sourceDir.is_absolute());
  EXPECT_TRUE(cfg.workDir.is_absolute());
  EXPECT_TRUE(cfg.compileDbDir.is_absolute());
  EXPECT_TRUE(cfg.outputDir.empty());
  EXPECT_TRUE(cfg.testResultDir.empty());
}

TEST_F(ConfigTest, testWithDefaultsSetsFieldDefaults) {
  auto cfg = Config::withDefaults();
  EXPECT_EQ("all", cfg.scope);
  EXPECT_EQ("uniform", cfg.generator);
  EXPECT_FALSE(cfg.timeout.has_value());
  EXPECT_EQ(60u, cfg.killAfter);
  EXPECT_EQ(0u, cfg.limit);
  EXPECT_TRUE(cfg.buildCmd.empty());
  EXPECT_TRUE(cfg.testCmd.empty());
}

TEST_F(ConfigTest, testYamlApplyOverwritesFields) {
  writeFile("sentinel.yaml", R"(
version: 1
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
build-command: make
test-command: ctest
test-result-dir: ./results
test-result-ext:
  - xml
coverage:
  - coverage.info
generator: random
timeout: 30
kill-after: 10
operator:
  - AOR
  - ROR
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_EQ((mTmpDir / "src").lexically_normal(), cfg.sourceDir);
  EXPECT_EQ((mTmpDir / "work").lexically_normal(), cfg.workDir);
  EXPECT_EQ((mTmpDir / "out").lexically_normal(), cfg.outputDir);
  EXPECT_EQ((mTmpDir / "build").lexically_normal(), cfg.compileDbDir);
  EXPECT_EQ("commit", cfg.scope);
  EXPECT_EQ(std::vector<std::string>({"cpp", "cxx"}), cfg.extensions);
  EXPECT_EQ(std::vector<std::string>({"src/**"}), cfg.patterns);
  EXPECT_EQ(std::vector<std::string>({"test/**"}), cfg.excludes);
  EXPECT_EQ("make", cfg.buildCmd);
  EXPECT_EQ("ctest", cfg.testCmd);
  EXPECT_EQ((mTmpDir / "results").lexically_normal(), cfg.testResultDir);
  EXPECT_EQ(std::vector<std::string>({"xml"}), cfg.testResultExts);
  EXPECT_EQ(std::vector<fs::path>({(mTmpDir / "coverage.info").lexically_normal()}), cfg.coverageFiles);
  EXPECT_EQ("random", cfg.generator);
  ASSERT_TRUE(cfg.timeout.has_value());
  EXPECT_EQ(30u, *cfg.timeout);
  EXPECT_EQ(10u, cfg.killAfter);
  EXPECT_EQ(std::vector<std::string>({"AOR", "ROR"}), cfg.operators);
}

TEST_F(ConfigTest, testYamlApplyPartialConfig) {
  writeFile("sentinel.yaml", R"(
version: 1
build-command: make all
test-command: ctest --verbose
test-result-dir: ./test-results
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_EQ("make all", cfg.buildCmd);
  EXPECT_EQ("ctest --verbose", cfg.testCmd);
  EXPECT_EQ((mTmpDir / "test-results").lexically_normal(), cfg.testResultDir);
  EXPECT_EQ("all", cfg.scope);
  EXPECT_EQ("uniform", cfg.generator);
  EXPECT_FALSE(cfg.timeout.has_value());
}

TEST_F(ConfigTest, testYamlApplyResolvesPathsRelativeToYamlFile) {
  writeFile("sentinel.yaml", R"(
version: 1
source-dir: ./src
workspace: ./.sentinel
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_EQ((mTmpDir / "src").lexically_normal(), cfg.sourceDir);
  EXPECT_EQ((mTmpDir / ".sentinel").lexically_normal(), cfg.workDir);
}

TEST_F(ConfigTest, testYamlIgnoresSeedLimitThresholdPartition) {
  writeFile("sentinel.yaml", R"(
version: 1
build-command: make
test-command: ctest
test-result-dir: ./results
seed: 42
limit: 100
threshold: 80.0
partition: 2/4
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_FALSE(cfg.seed.has_value());
  EXPECT_EQ(0u, cfg.limit);
  EXPECT_FALSE(cfg.threshold.has_value());
  EXPECT_FALSE(cfg.partition.has_value());
}

TEST_F(ConfigTest, testYamlTimeoutAsNumberIsAccepted) {
  writeFile("sentinel.yaml", R"(
version: 1
timeout: 45
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  ASSERT_TRUE(cfg.timeout.has_value());
  EXPECT_EQ(45u, *cfg.timeout);
}

TEST_F(ConfigTest, testYamlEmptyConfigThrows) {
  writeFile("empty.yaml", "");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("empty.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testYamlInvalidSyntaxThrows) {
  writeFile("bad.yaml", "key: [unclosed");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("bad.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testYamlMissingVersionThrows) {
  writeFile("no-version.yaml", "build-command: make\n");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("no-version.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testYamlUnsupportedVersionThrows) {
  writeFile("future.yaml", "version: 99\nbuild-command: make\n");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("future.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testYamlVersionAsNonIntegerThrows) {
  writeFile("bad-version.yaml", "version: \"not_an_integer\"\nbuild-command: make\n");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("bad-version.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testYamlListFieldAsScalarThrows) {
  writeFile("bad-extension.yaml", "version: 1\nextension: \"cpp\"\n");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("bad-extension.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testYamlFieldWithWrongTypeThrows) {
  writeFile("bad-type.yaml", "version: 1\ntimeout: [1, 2]\n");
  Config cfg;
  EXPECT_THROW(YamlConfigParser::applyTo(&cfg, configPath("bad-type.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testPartitionParseValid) {
  auto p = Partition::parse("2/4");
  EXPECT_EQ(2u, p.index);
  EXPECT_EQ(4u, p.count);
}

TEST_F(ConfigTest, testPartitionParseSinglePartition) {
  auto p = Partition::parse("1/1");
  EXPECT_EQ(1u, p.index);
  EXPECT_EQ(1u, p.count);
}

TEST_F(ConfigTest, testPartitionParseThrowsOnMissingSlash) {
  EXPECT_THROW(Partition::parse("bad"), std::invalid_argument);
}

TEST_F(ConfigTest, testPartitionParseThrowsOnSlashAtStart) {
  EXPECT_THROW(Partition::parse("/4"), std::invalid_argument);
}

TEST_F(ConfigTest, testPartitionParseThrowsOnSlashAtEnd) {
  EXPECT_THROW(Partition::parse("1/"), std::invalid_argument);
}

TEST_F(ConfigTest, testPartitionParseThrowsOnNonNumeric) {
  EXPECT_THROW(Partition::parse("abc/def"), std::invalid_argument);
}

TEST_F(ConfigTest, testPartitionParseThrowsWhenIndexIsZero) {
  EXPECT_THROW(Partition::parse("0/4"), std::invalid_argument);
}

TEST_F(ConfigTest, testPartitionParseThrowsWhenCountIsZero) {
  EXPECT_THROW(Partition::parse("1/0"), std::invalid_argument);
}

TEST_F(ConfigTest, testPartitionParseThrowsWhenIndexExceedsCount) {
  EXPECT_THROW(Partition::parse("5/4"), std::invalid_argument);
}

}  // namespace sentinel
