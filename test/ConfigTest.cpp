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

  ASSERT_TRUE(cfg.testResultFileExts.has_value());
  EXPECT_EQ(std::vector<std::string>({"xml"}), *cfg.testResultFileExts);

  ASSERT_TRUE(cfg.coverageFiles.has_value());
  EXPECT_EQ(std::vector<std::filesystem::path>({(mTmpDir / "coverage.info").lexically_normal()}), *cfg.coverageFiles);

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

TEST_F(ConfigTest, testLoadPartialConfig) {
  writeFile("sentinel.yaml", R"(
version: 1
build-command: make all
test-command: ctest --verbose
test-report-dir: ./test-results
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
  EXPECT_EQ((std::filesystem::current_path() / "sentinel_workspace").lexically_normal(), *cfg.workDir);
  EXPECT_EQ("auto", *cfg.timeLimit);
  EXPECT_EQ(0u, *cfg.limit);
  EXPECT_EQ("uniform", *cfg.generator);
}

}  // namespace sentinel
