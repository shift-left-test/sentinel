/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "helper/ThrowMessageMatcher.hpp"
#include "sentinel/CliConfigParser.hpp"

using ::testing::HasSubstr;

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
  EXPECT_FALSE(cfg.from.has_value());
  EXPECT_FALSE(cfg.uncommitted);
  EXPECT_EQ(Generator::UNIFORM, cfg.generator);
  EXPECT_FALSE(cfg.timeout.has_value());
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
  EXPECT_THROW_MESSAGE(
      parse({"--seed", "auto"}),
      std::invalid_argument, HasSubstr("--seed"));
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
  EXPECT_THROW_MESSAGE(
      parse({"--timeout", "auto"}),
      std::invalid_argument, HasSubstr("--timeout"));
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
  Config cfg = parse({"--from", "HEAD~1", "--generator", "random"});
  ASSERT_TRUE(cfg.from.has_value());
  EXPECT_EQ("HEAD~1", *cfg.from);
  EXPECT_EQ(Generator::RANDOM, cfg.generator);
}

TEST_F(CliConfigParserTest, testFromParsed) {
  Config cfg = parse({"--from", "main"});
  ASSERT_TRUE(cfg.from.has_value());
  EXPECT_EQ("main", *cfg.from);
}

TEST_F(CliConfigParserTest, testUncommittedParsed) {
  Config cfg = parse({"--uncommitted"});
  EXPECT_TRUE(cfg.uncommitted);
}

TEST_F(CliConfigParserTest, testUncommittedDefaultFalse) {
  Config cfg = parse({});
  EXPECT_FALSE(cfg.uncommitted);
}

TEST_F(CliConfigParserTest, testFromAndUncommittedCombined) {
  Config cfg = parse({"--from", "HEAD~2", "--uncommitted"});
  ASSERT_TRUE(cfg.from.has_value());
  EXPECT_EQ("HEAD~2", *cfg.from);
  EXPECT_TRUE(cfg.uncommitted);
}

TEST_F(CliConfigParserTest, testWorkspaceParsedAndAbsolute) {
  Config cfg = parse({"--workspace", "ws"});
  EXPECT_TRUE(cfg.workDir.is_absolute());
  EXPECT_NE(cfg.workDir.string().find("ws"), std::string::npos);
}

TEST_F(CliConfigParserTest, testCompileDbDirParsedAndAbsolute) {
  Config cfg = parse({"--compiledb-dir", "build"});
  EXPECT_TRUE(cfg.compileDbDir.is_absolute());
  EXPECT_NE(cfg.compileDbDir.string().find("build"), std::string::npos);
}

TEST_F(CliConfigParserTest, testTestResultDirParsedAndAbsolute) {
  Config cfg = parse({"--test-result-dir", "results"});
  EXPECT_TRUE(cfg.testResultDir.is_absolute());
  EXPECT_NE(cfg.testResultDir.string().find("results"), std::string::npos);
}

TEST_F(CliConfigParserTest, testBuildCommandParsed) {
  Config cfg = parse({"--build-command", "make all -j4"});
  EXPECT_EQ(cfg.buildCmd, "make all -j4");
}

TEST_F(CliConfigParserTest, testTestCommandParsed) {
  Config cfg = parse({"--test-command", "ctest -j8"});
  EXPECT_EQ(cfg.testCmd, "ctest -j8");
}

TEST_F(CliConfigParserTest, testCoverageFileParsed) {
  Config cfg = parse({"--coverage", "cov1.info", "--coverage", "cov2.info"});
  EXPECT_EQ(cfg.coverageFiles.size(), 2u);
}

TEST_F(CliConfigParserTest, testSourceDirParsedAndAbsolute) {
  Config cfg = parse({"--source-dir", "src"});
  EXPECT_TRUE(cfg.sourceDir.is_absolute());
}

TEST_F(CliConfigParserTest, testDryRunParsed) {
  Config cfg = parse({"--dry-run"});
  EXPECT_TRUE(cfg.dryRun);
}

TEST_F(CliConfigParserTest, testInitParsed) {
  Config cfg = parse({"--init"});
  EXPECT_TRUE(cfg.init);
}

TEST_F(CliConfigParserTest, testPartitionParsed) {
  Config cfg = parse({"--partition", "2/4"});
  ASSERT_TRUE(cfg.partition.has_value());
  EXPECT_EQ(*cfg.partition, "2/4");
}

TEST_F(CliConfigParserTest, testPatternParsed) {
  Config cfg = parse({"--pattern", "src/**/*.cpp", "--pattern", "lib/**/*.cpp"});
  EXPECT_EQ(cfg.patterns.size(), 2u);
}

TEST_F(CliConfigParserTest, testLimitNonNumericThrows) {
  EXPECT_THROW(parse({"--limit", "abc"}), args::ParseError);
}

TEST_F(CliConfigParserTest, testCleanShortOptionParsed) {
  Config cfg = parse({"-c"});
  EXPECT_TRUE(cfg.clean);
}

TEST_F(CliConfigParserTest, testDryRunShortOptionParsed) {
  Config cfg = parse({"-n"});
  EXPECT_TRUE(cfg.dryRun);
}

TEST_F(CliConfigParserTest, testVerboseShortOptionParsed) {
  Config cfg = parse({"-v"});
  EXPECT_TRUE(cfg.verbose);
}

TEST_F(CliConfigParserTest, testExtensionShortOptionRemoved) {
  EXPECT_THROW(parse({"-t", "cpp"}), args::ParseError);
}

TEST_F(CliConfigParserTest, testMergePartitionParsed) {
  Config cfg = parse({"--merge-partition", "/data/part1", "--merge-partition", "/data/part2"});
  ASSERT_EQ(cfg.mergeWorkspaces.size(), 2u);
  EXPECT_TRUE(cfg.mergeWorkspaces[0].is_absolute());
  EXPECT_TRUE(cfg.mergeWorkspaces[1].is_absolute());
}

TEST_F(CliConfigParserTest, testMergePartitionDefaultsToEmpty) {
  Config cfg = parse({});
  EXPECT_TRUE(cfg.mergeWorkspaces.empty());
}

TEST_F(CliConfigParserTest, testApplyReportOnlyToAppliesThreshold) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--threshold", "80.5"});
  Config cfg = Config::withDefaults();
  cliParser.applyReportOnlyTo(&cfg);
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(*cfg.threshold, 80.5);
}

TEST_F(CliConfigParserTest, testApplyReportOnlyToAppliesOutputDir) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--output-dir", "report"});
  Config cfg = Config::withDefaults();
  cliParser.applyReportOnlyTo(&cfg);
  EXPECT_TRUE(cfg.outputDir.is_absolute());
}

TEST_F(CliConfigParserTest, testApplyReportOnlyToAppliesVerbose) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--verbose"});
  Config cfg = Config::withDefaults();
  cliParser.applyReportOnlyTo(&cfg);
  EXPECT_TRUE(cfg.verbose);
}

TEST_F(CliConfigParserTest, testApplyReportOnlyToIgnoresFrom) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--from", "HEAD~1", "--threshold", "80"});
  Config cfg = Config::withDefaults();
  cliParser.applyReportOnlyTo(&cfg);
  EXPECT_FALSE(cfg.from.has_value());
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(*cfg.threshold, 80.0);
}

TEST_F(CliConfigParserTest, testApplyReportOnlyToIgnoresBuildCommand) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--build-command", "make -j8"});
  Config cfg = Config::withDefaults();
  cliParser.applyReportOnlyTo(&cfg);
  EXPECT_TRUE(cfg.buildCmd.empty());
}

TEST_F(CliConfigParserTest, testApplyReportOnlyToIgnoresTimeout) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--timeout", "60"});
  Config cfg = Config::withDefaults();
  cliParser.applyReportOnlyTo(&cfg);
  EXPECT_FALSE(cfg.timeout.has_value());
}

TEST_F(CliConfigParserTest, testEffectiveCliOptionsEmpty) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{});
  EXPECT_TRUE(cliParser.getEffectiveCliOptions().empty());
}

TEST_F(CliConfigParserTest, testEffectiveCliOptionsSingleFlag) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--from", "HEAD~1"});
  auto opts = cliParser.getEffectiveCliOptions();
  EXPECT_EQ(opts.size(), 1u);
  EXPECT_EQ(opts[0], "--from");
}

TEST_F(CliConfigParserTest, testEffectiveCliOptionsMultipleFlags) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--from", "HEAD~1", "--timeout", "30", "--threshold", "80"});
  auto opts = cliParser.getEffectiveCliOptions();
  EXPECT_EQ(opts.size(), 2u);
  EXPECT_THAT(opts, ::testing::UnorderedElementsAre("--from", "--timeout"));
}

TEST_F(CliConfigParserTest, testEffectiveCliOptionsExcludesControlFlags) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--verbose", "--clean", "--dry-run", "--from", "main"});
  auto opts = cliParser.getEffectiveCliOptions();
  EXPECT_THAT(opts, ::testing::UnorderedElementsAre("--from"));
}

TEST_F(CliConfigParserTest, testEffectiveCliOptionsUncommitted) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--uncommitted"});
  auto opts = cliParser.getEffectiveCliOptions();
  EXPECT_EQ(opts.size(), 1u);
  EXPECT_EQ(opts[0], "--uncommitted");
}

TEST_F(CliConfigParserTest, testResumeFlowIgnoresFromButAppliesThreshold) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--from", "HEAD~1", "--threshold", "80"});

  // Simulate resume: start from defaults (as if workspace config was loaded)
  Config cfg = Config::withDefaults();
  cfg.from = "HEAD~3";  // pretend workspace had HEAD~3

  cliParser.applyReportOnlyTo(&cfg);

  // --from should NOT be overwritten
  ASSERT_TRUE(cfg.from.has_value());
  EXPECT_EQ(*cfg.from, "HEAD~3");

  // --threshold SHOULD be applied
  ASSERT_TRUE(cfg.threshold.has_value());
  EXPECT_DOUBLE_EQ(*cfg.threshold, 80.0);

  // --from should appear in ignored list
  auto ignored = cliParser.getEffectiveCliOptions();
  EXPECT_THAT(ignored, ::testing::Contains("--from"));
  EXPECT_THAT(ignored, ::testing::Not(::testing::Contains("--threshold")));
}

TEST_F(CliConfigParserTest, testResumeFlowNoWarningWhenOnlyReportOptions) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--threshold", "80", "--output-dir", "out", "--verbose"});

  auto ignored = cliParser.getEffectiveCliOptions();
  EXPECT_TRUE(ignored.empty());
}

TEST_F(CliConfigParserTest, testResumeFlowWarnsAllNonReportOptions) {
  args::ArgumentParser parser("test", "");
  CliConfigParser cliParser(parser);
  parser.ParseArgs(std::vector<std::string>{"--from", "main", "--uncommitted", "--timeout", "60",
                     "--operator", "AOR", "--threshold", "80"});

  auto ignored = cliParser.getEffectiveCliOptions();
  EXPECT_THAT(ignored, ::testing::UnorderedElementsAre(
      "--from", "--uncommitted", "--timeout", "--operator"));
}

}  // namespace sentinel
