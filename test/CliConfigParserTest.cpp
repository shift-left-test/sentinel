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
    Config cfg = Config::withDefaults();
    cliParser.applyTo(&cfg);
    return cfg;
  }

  std::filesystem::path configFile(const std::vector<std::string>& args) {
    args::ArgumentParser parser("test", "");
    CliConfigParser cliParser(parser);
    parser.ParseArgs(args);
    return cliParser.getConfigFile();
  }
};

TEST_F(CliConfigParserTest, testDefaultsPreservedWhenNoCli) {
  Config cfg = parse({});
  EXPECT_EQ("all", cfg.scope);
  EXPECT_EQ("uniform", cfg.generator);
  EXPECT_FALSE(cfg.timeout.has_value());
  EXPECT_EQ(60u, cfg.killAfter);
  EXPECT_EQ(0u, cfg.limit);
  EXPECT_FALSE(cfg.seed.has_value());
  EXPECT_FALSE(cfg.threshold.has_value());
  EXPECT_FALSE(cfg.init);
  EXPECT_FALSE(cfg.dryRun);
  EXPECT_FALSE(cfg.verbose);
}

TEST_F(CliConfigParserTest, testOutputDirResolvedToAbsolute) {
  Config cfg = parse({"--output-dir", "out"});
  EXPECT_TRUE(cfg.outputDir.is_absolute());
}

TEST_F(CliConfigParserTest, testThresholdParsed) {
  Config cfg = parse({"--threshold", "80.5"});
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(*cfg.threshold, 80.5);
}

TEST_F(CliConfigParserTest, testOperatorsListParsed) {
  Config cfg = parse({"--operator", "AOR", "--operator", "BOR"});
  EXPECT_EQ(cfg.operators.size(), 2u);
  EXPECT_EQ(cfg.operators[0], "AOR");
  EXPECT_EQ(cfg.operators[1], "BOR");
}

TEST_F(CliConfigParserTest, testExtensionListParsed) {
  Config cfg = parse({"--extension", "cpp", "--extension", "cxx"});
  EXPECT_EQ(cfg.extensions.size(), 2u);
  EXPECT_EQ(cfg.extensions[0], "cpp");
  EXPECT_EQ(cfg.extensions[1], "cxx");
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

TEST_F(CliConfigParserTest, testSeedNonNumericThrows) {
  EXPECT_THROW(parse({"--seed", "auto"}), std::invalid_argument);
}

TEST_F(CliConfigParserTest, testLimitParsed) {
  Config cfg = parse({"--limit", "50"});
  EXPECT_EQ(cfg.limit, 50u);
}

TEST_F(CliConfigParserTest, testTimeoutNumericParsed) {
  Config cfg = parse({"--timeout", "30"});
  ASSERT_TRUE(cfg.timeout.has_value());
  EXPECT_EQ(*cfg.timeout, 30u);
}

TEST_F(CliConfigParserTest, testTimeoutNonNumericThrows) {
  EXPECT_THROW(parse({"--timeout", "auto"}), std::invalid_argument);
}

TEST_F(CliConfigParserTest, testKillAfterParsed) {
  Config cfg = parse({"--kill-after", "30"});
  EXPECT_EQ(cfg.killAfter, 30u);
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

TEST_F(CliConfigParserTest, testCliOverwritesDefaults) {
  Config cfg = parse({"--scope", "commit", "--generator", "random"});
  EXPECT_EQ("commit", cfg.scope);
  EXPECT_EQ("random", cfg.generator);
}

}  // namespace sentinel
