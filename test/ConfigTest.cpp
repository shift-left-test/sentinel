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
#include "sentinel/Config.hpp"
#include "sentinel/ConfigResolver.hpp"
#include "sentinel/YamlConfigParser.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
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

  std::filesystem::path configPath(const std::string& filename) {
    return mTmpDir / filename;
  }

  std::filesystem::path mTmpDir;
};

TEST_F(ConfigTest, testLoadCompleteConfig) {
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
limit: 5
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
seed: 42
operator:
  - AOR
  - ROR
threshold: 75.5
partition: 2/4
)");

  auto yamlCfg = YamlConfigParser::loadFromFile(configPath("sentinel.yaml"));
  auto cfg = ConfigResolver::resolve(Config(), yamlCfg, configPath("sentinel.yaml"));

  ASSERT_TRUE(cfg.sourceDir.has_value());
  EXPECT_EQ((mTmpDir / "src").lexically_normal(), *cfg.sourceDir);

  ASSERT_TRUE(cfg.workDir.has_value());
  EXPECT_EQ((mTmpDir / "work").lexically_normal(), *cfg.workDir);

  ASSERT_TRUE(cfg.outputDir.has_value());
  EXPECT_EQ((mTmpDir / "out").lexically_normal(), *cfg.outputDir);

  ASSERT_TRUE(cfg.compileDbDir.has_value());
  EXPECT_EQ((mTmpDir / "build").lexically_normal(), *cfg.compileDbDir);

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
  EXPECT_EQ((mTmpDir / "results").lexically_normal(), *cfg.testResultDir);

  ASSERT_TRUE(cfg.testResultExts.has_value());
  EXPECT_EQ(std::vector<std::string>({"xml"}), *cfg.testResultExts);

  ASSERT_TRUE(cfg.coverageFiles.has_value());
  EXPECT_EQ(std::vector<std::filesystem::path>({(mTmpDir / "coverage.info").lexically_normal()}), *cfg.coverageFiles);

  ASSERT_TRUE(cfg.generator.has_value());
  EXPECT_EQ("random", *cfg.generator);

  ASSERT_TRUE(cfg.timeout.has_value());
  EXPECT_EQ("30", *cfg.timeout);

  ASSERT_TRUE(cfg.killAfter.has_value());
  EXPECT_EQ("10", *cfg.killAfter);

  ASSERT_TRUE(cfg.seed.has_value());
  EXPECT_EQ(42u, *cfg.seed);

  ASSERT_TRUE(cfg.operators.has_value());
  EXPECT_EQ(std::vector<std::string>({"AOR", "ROR"}), *cfg.operators);

  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(75.5, *cfg.threshold);

  ASSERT_TRUE(cfg.partition.has_value());
  EXPECT_EQ("2/4", *cfg.partition);
}

TEST_F(ConfigTest, testLoadPartialConfig) {
  writeFile("sentinel.yaml", R"(
version: 1
build-command: make all
test-command: ctest --verbose
test-result-dir: ./test-results
limit: 20
)");

  auto yamlCfg = YamlConfigParser::loadFromFile(configPath("sentinel.yaml"));
  auto cfg = ConfigResolver::resolve(Config(), yamlCfg, configPath("sentinel.yaml"));

  ASSERT_TRUE(cfg.buildCmd.has_value());
  EXPECT_EQ("make all", *cfg.buildCmd);

  ASSERT_TRUE(cfg.testCmd.has_value());
  EXPECT_EQ("ctest --verbose", *cfg.testCmd);

  ASSERT_TRUE(cfg.testResultDir.has_value());
  EXPECT_EQ((mTmpDir / "test-results").lexically_normal(), *cfg.testResultDir);

  ASSERT_TRUE(cfg.limit.has_value());
  EXPECT_EQ(20u, *cfg.limit);
}

TEST_F(ConfigTest, testSentinelYamlParsesThresholdAndPartition) {
  writeFile("sentinel.yaml", R"(
version: 1
build-command: make
test-command: ctest
test-result-dir: ./results
threshold: 80.0
partition: 1/3
)");

  auto yamlCfg = YamlConfigParser::loadFromFile(configPath("sentinel.yaml"));
  auto cfg = ConfigResolver::resolve(Config(), yamlCfg, configPath("sentinel.yaml"));

  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(80.0, *cfg.threshold);

  ASSERT_TRUE(cfg.partition.has_value());
  EXPECT_EQ("1/3", *cfg.partition);
}

TEST_F(ConfigTest, testWorkspaceConfigRoundTrip) {
  // Simulate what buildWorkspaceYaml writes when threshold and partition are set
  writeFile("config.yaml",
            "version: 1\n"
            "source-dir: /tmp/src\n"
            "compiledb-dir: /tmp\n"
            "scope: all\n"
            "extension:\n"
            "  - cpp\n"
            "pattern: []\n"
            "exclude: []\n"
            "limit: 0\n"
            "build-command: make\n"
            "test-command: ctest\n"
            "test-result-dir: /tmp/results\n"
            "test-result-ext:\n"
            "  - xml\n"
            "coverage: []\n"
            "generator: uniform\n"
            "timeout: 60\n"
            "kill-after: 60\n"
            "operator: []\n"
            "threshold: 85.5\n"
            "partition: 2/4\n");

  auto cfg = YamlConfigParser::loadFromFile(configPath("config.yaml"));

  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(85.5, *cfg.threshold);

  ASSERT_TRUE(cfg.partition.has_value());
  EXPECT_EQ("2/4", *cfg.partition);
}

TEST_F(ConfigTest, testLoadEmptyConfig) {
  writeFile("empty.yaml", "");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("empty.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testLoadInvalidYamlSyntax) {
  writeFile("bad.yaml", "key: [unclosed");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("bad.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testLoadMissingVersion) {
  writeFile("no-version.yaml", "build-command: make\n");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("no-version.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testLoadUnsupportedVersion) {
  writeFile("future.yaml", "version: 99\nbuild-command: make\n");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("future.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testResolvePriority) {
  Config cli;
  cli.limit = 100;
  cli.buildCmd = "make cli";

  Config yaml;
  yaml.limit = 50;
  yaml.buildCmd = "make yaml";
  yaml.testCmd = "ctest yaml";

  auto cfg = ConfigResolver::resolve(cli, yaml);

  EXPECT_EQ(100u, *cfg.limit);
  EXPECT_EQ("make cli", *cfg.buildCmd);
  EXPECT_EQ("ctest yaml", *cfg.testCmd);
}

TEST_F(ConfigTest, testResolveDefaults) {
  auto cfg = ConfigResolver::resolve(Config(), Config());

  EXPECT_EQ(std::filesystem::current_path().lexically_normal().string() + "/", *cfg.sourceDir);
  EXPECT_EQ((std::filesystem::current_path() / ".sentinel").lexically_normal(), *cfg.workDir);
  EXPECT_EQ("auto", *cfg.timeout);
  EXPECT_EQ(0u, *cfg.limit);
  EXPECT_EQ("uniform", *cfg.generator);
}

TEST_F(ConfigTest, testVersionAsNonIntegerThrows) {
  writeFile("bad-version.yaml", "version: \"not_an_integer\"\nbuild-command: make\n");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("bad-version.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testListFieldAsScalarThrows) {
  // 'extension' expects a sequence; giving a scalar should throw
  writeFile("bad-extension.yaml", "version: 1\nextension: \"cpp\"\n");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("bad-extension.yaml")), std::runtime_error);
}

TEST_F(ConfigTest, testFieldWithWrongTypeThrows) {
  // 'limit' expects an integer; giving a non-numeric string should throw
  writeFile("bad-limit.yaml", "version: 1\nlimit: \"not_a_number\"\n");
  EXPECT_THROW(YamlConfigParser::loadFromFile(configPath("bad-limit.yaml")), std::runtime_error);
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

TEST_F(ConfigTest, testToAbsolutePathsResolvesYamlRelativePaths) {
  Config cfg;
  cfg.sourceDir = fs::path("src");
  cfg.workDir = fs::path(".sentinel");

  cfg.toAbsolutePaths(mTmpDir / "sentinel.yaml", Config{});

  EXPECT_EQ((mTmpDir / "src").lexically_normal(), *cfg.sourceDir);
  EXPECT_EQ((mTmpDir / ".sentinel").lexically_normal(), *cfg.workDir);
}

TEST_F(ConfigTest, testToAbsolutePathsResolvesCliRelativePathsFromCwd) {
  Config cfg;
  cfg.sourceDir = fs::path("src");

  Config cliConfig;
  cliConfig.sourceDir = fs::path("src");

  cfg.toAbsolutePaths(mTmpDir / "sentinel.yaml", cliConfig);

  EXPECT_EQ(fs::absolute("src").lexically_normal(), *cfg.sourceDir);
}

TEST_F(ConfigTest, testToAbsolutePathsLeavesAbsolutePathsUnchanged) {
  Config cfg;
  cfg.sourceDir = mTmpDir / "src";

  cfg.toAbsolutePaths(mTmpDir / "sentinel.yaml", Config{});

  EXPECT_EQ((mTmpDir / "src").lexically_normal(), *cfg.sourceDir);
}

TEST_F(ConfigTest, testToAbsolutePathsResolvesCoverageFiles) {
  Config cfg;
  cfg.coverageFiles = std::vector<fs::path>{"coverage.info"};

  cfg.toAbsolutePaths(mTmpDir / "sentinel.yaml", Config{});

  EXPECT_EQ((mTmpDir / "coverage.info").lexically_normal(), (*cfg.coverageFiles)[0]);
}

}  // namespace sentinel
