/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/CliConfigParser.hpp"

namespace sentinel {

class CliConfigParserTest : public ::testing::Test {
 protected:
  Config parse(const std::vector<std::string>& args) {
    args::ArgumentParser parser("test", "");
    CliConfigParser cliParser(parser);
    parser.ParseArgs(args);
    return cliParser.getConfig();
  }

  std::filesystem::path configFile(const std::vector<std::string>& args) {
    args::ArgumentParser parser("test", "");
    CliConfigParser cliParser(parser);
    parser.ParseArgs(args);
    return cliParser.getConfigFile();
  }
};

TEST_F(CliConfigParserTest, testDefaultConfigIsAllNullopt) {
  Config cfg = parse({});
  EXPECT_FALSE(cfg.sourceDir.has_value());
  EXPECT_FALSE(cfg.workDir.has_value());
  EXPECT_FALSE(cfg.outputDir.has_value());
  EXPECT_FALSE(cfg.buildCmd.has_value());
  EXPECT_FALSE(cfg.testCmd.has_value());
  EXPECT_FALSE(cfg.limit.has_value());
  EXPECT_FALSE(cfg.seed.has_value());
  EXPECT_FALSE(cfg.threshold.has_value());
  EXPECT_FALSE(cfg.extensions.has_value());
  EXPECT_FALSE(cfg.operators.has_value());
  // Non-optional bool fields default to false
  EXPECT_FALSE(cfg.init);
  EXPECT_FALSE(cfg.dryRun);
  EXPECT_FALSE(cfg.noStatusLine);
  EXPECT_FALSE(cfg.verbose);
  EXPECT_FALSE(cfg.force);
  EXPECT_FALSE(cfg.clean);
}

TEST_F(CliConfigParserTest, testOutputDirParsed) {
  Config cfg = parse({"--output-dir", "/tmp/out"});
  ASSERT_TRUE(cfg.outputDir.has_value());
  EXPECT_EQ(cfg.outputDir->string(), "/tmp/out");
}

TEST_F(CliConfigParserTest, testThresholdParsed) {
  Config cfg = parse({"--threshold", "80.5"});
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(*cfg.threshold, 80.5);
}

TEST_F(CliConfigParserTest, testOperatorsListParsed) {
  Config cfg = parse({"--operator", "AOR", "--operator", "BOR"});
  ASSERT_TRUE(cfg.operators.has_value());
  EXPECT_EQ(cfg.operators->size(), 2u);
  EXPECT_EQ((*cfg.operators)[0], "AOR");
  EXPECT_EQ((*cfg.operators)[1], "BOR");
}

TEST_F(CliConfigParserTest, testExtensionListParsed) {
  Config cfg = parse({"--extension", "cpp", "--extension", "cxx"});
  ASSERT_TRUE(cfg.extensions.has_value());
  EXPECT_EQ(cfg.extensions->size(), 2u);
  EXPECT_EQ((*cfg.extensions)[0], "cpp");
  EXPECT_EQ((*cfg.extensions)[1], "cxx");
}

TEST_F(CliConfigParserTest, testConfigFileParsed) {
  auto path = configFile({"--config", "sentinel.yaml"});
  EXPECT_EQ(path.string(), "sentinel.yaml");
}

TEST_F(CliConfigParserTest, testSeedNumericParsed) {
  Config cfg = parse({"--seed", "1234"});
  ASSERT_TRUE(cfg.seed.has_value());
  EXPECT_EQ(*cfg.seed, 1234u);
}

TEST_F(CliConfigParserTest, testSeedAutoParsed) {
  // "--seed auto" means "pick randomly" — seed stays nullopt
  Config cfg = parse({"--seed", "auto"});
  EXPECT_FALSE(cfg.seed.has_value());
}

TEST_F(CliConfigParserTest, testLimitParsed) {
  Config cfg = parse({"--limit", "50"});
  ASSERT_TRUE(cfg.limit.has_value());
  EXPECT_EQ(*cfg.limit, 50u);
}

TEST_F(CliConfigParserTest, testVerboseParsed) {
  Config cfg = parse({"--verbose"});
  EXPECT_TRUE(cfg.verbose);
}

TEST_F(CliConfigParserTest, testForceParsed) {
  Config cfg = parse({"--force"});
  EXPECT_TRUE(cfg.force);
}

TEST_F(CliConfigParserTest, testCleanParsed) {
  Config cfg = parse({"--clean"});
  EXPECT_TRUE(cfg.clean);
}

}  // namespace sentinel
