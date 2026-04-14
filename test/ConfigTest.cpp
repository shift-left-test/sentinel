/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "helper/ThrowMessageMatcher.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/YamlConfigParser.hpp"

namespace fs = std::filesystem;

using ::testing::HasSubstr;

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
  EXPECT_FALSE(cfg.from.has_value());
  EXPECT_FALSE(cfg.uncommitted);
  EXPECT_EQ(Generator::UNIFORM, cfg.generator);
  EXPECT_FALSE(cfg.timeout.has_value());
  EXPECT_EQ(0u, cfg.limit);
  EXPECT_EQ(1u, cfg.mutantsPerLine);
  EXPECT_TRUE(cfg.buildCmd.empty());
  EXPECT_TRUE(cfg.testCmd.empty());
}

TEST_F(ConfigTest, testYamlApplyOverwritesFields) {
  writeFile("sentinel.yaml", R"(
version: 1
source-dir: ./src
output-dir: ./out
compiledb-dir: ./build
extension:
  - cpp
  - cxx
pattern:
  - src/**
  - "!test/**"
build-command: make
test-command: ctest
test-result-dir: ./results
lcov-tracefile:
  - coverage.info
generator: random
mutants-per-line: 3
timeout: 30
operator:
  - AOR
  - ROR
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_EQ((mTmpDir / "src").lexically_normal(), cfg.sourceDir);
  EXPECT_EQ((mTmpDir / "out").lexically_normal(), cfg.outputDir);
  EXPECT_EQ((mTmpDir / "build").lexically_normal(), cfg.compileDbDir);
  EXPECT_FALSE(cfg.from.has_value());
  EXPECT_EQ(std::vector<std::string>({"cpp", "cxx"}), cfg.extensions);
  EXPECT_EQ(std::vector<std::string>({"src/**", "!test/**"}), cfg.patterns);
  EXPECT_EQ("make", cfg.buildCmd);
  EXPECT_EQ("ctest", cfg.testCmd);
  EXPECT_EQ((mTmpDir / "results").lexically_normal(), cfg.testResultDir);
  EXPECT_EQ(std::vector<fs::path>({(mTmpDir / "coverage.info").lexically_normal()}), cfg.lcovTracefiles);
  EXPECT_EQ(Generator::RANDOM, cfg.generator);
  EXPECT_EQ(3u, cfg.mutantsPerLine);
  ASSERT_TRUE(cfg.timeout.has_value());
  EXPECT_EQ(30u, *cfg.timeout);
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
  EXPECT_FALSE(cfg.from.has_value());
  EXPECT_EQ(Generator::UNIFORM, cfg.generator);
  EXPECT_EQ(1u, cfg.mutantsPerLine);
  EXPECT_FALSE(cfg.timeout.has_value());
}

TEST_F(ConfigTest, testYamlApplyResolvesPathsRelativeToYamlFile) {
  writeFile("sentinel.yaml", R"(
version: 1
source-dir: ./src
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_EQ((mTmpDir / "src").lexically_normal(), cfg.sourceDir);
}

TEST_F(ConfigTest, testYamlIgnoresCliOnlyOptions) {
  writeFile("sentinel.yaml", R"(
version: 1
build-command: make
test-command: ctest
test-result-dir: ./results
from: HEAD~1
seed: 42
limit: 100
threshold: 80.0
partition: 2/4
)");

  Config cfg = Config::withDefaults();
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_FALSE(cfg.from.has_value());
  EXPECT_FALSE(cfg.seed.has_value());
  EXPECT_EQ(0u, cfg.limit);
  EXPECT_FALSE(cfg.threshold.has_value());
  EXPECT_FALSE(cfg.partition.has_value());
}

TEST_F(ConfigTest, testYamlIgnoresWorkspace) {
  writeFile("sentinel.yaml", R"(
version: 1
workspace: ./custom-ws
build-command: make
)");

  Config cfg = Config::withDefaults();
  auto originalWorkDir = cfg.workDir;
  YamlConfigParser::applyTo(&cfg, configPath("sentinel.yaml"));

  EXPECT_EQ(originalWorkDir, cfg.workDir);
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

TEST_F(ConfigTest, testYamlNewerVersionSuggestsUpgrade) {
  writeFile("future.yaml", "version: 99\nbuild-command: make\n");
  Config cfg;
  EXPECT_THROW_MESSAGE(
      YamlConfigParser::applyTo(&cfg, configPath("future.yaml")),
      std::runtime_error, HasSubstr("upgrade sentinel"));
}

TEST_F(ConfigTest, testYamlOlderVersionSuggestsRegenerate) {
  writeFile("old.yaml", "version: 0\nbuild-command: make\n");
  Config cfg;
  EXPECT_THROW_MESSAGE(
      YamlConfigParser::applyTo(&cfg, configPath("old.yaml")),
      std::runtime_error, HasSubstr("sentinel --init"));
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

TEST_F(ConfigTest, testStreamOperatorWritesYaml) {
  Config cfg = Config::withDefaults();
  cfg.buildCmd = "make -j8";
  cfg.testCmd = "ctest";
  cfg.generator = Generator::UNIFORM;
  cfg.extensions = {"cpp", "cc"};
  cfg.operators = {"AOR", "ROR"};

  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("version: 1"));
  EXPECT_NE(std::string::npos, yaml.find("build-command: make -j8"));
  EXPECT_EQ(std::string::npos, yaml.find("from:"));
}

TEST_F(ConfigTest, testStreamOperatorRoundTripViaYamlParser) {
  Config original = Config::withDefaults();
  original.sourceDir = fs::absolute("src");
  original.compileDbDir = fs::absolute("build");
  original.buildCmd = "make";
  original.testCmd = "ctest";
  original.testResultDir = fs::absolute("results");
  original.generator = Generator::RANDOM;
  original.timeout = 120;
  original.mutantsPerLine = 5;
  original.extensions = {"cpp"};
  original.operators = {"AOR"};

  auto tmpFile = mTmpDir / "config_roundtrip.yaml";
  {
    std::ofstream out(tmpFile);
    out << original;
  }

  Config loaded = Config::withDefaults();
  YamlConfigParser::applyTo(&loaded, tmpFile);

  EXPECT_EQ(original.sourceDir, loaded.sourceDir);
  EXPECT_EQ(original.buildCmd, loaded.buildCmd);
  EXPECT_EQ(original.testCmd, loaded.testCmd);
  EXPECT_FALSE(loaded.from.has_value());
  EXPECT_EQ(original.generator, loaded.generator);
  ASSERT_TRUE(loaded.timeout.has_value());
  EXPECT_EQ(*original.timeout, *loaded.timeout);
  EXPECT_EQ(original.mutantsPerLine, loaded.mutantsPerLine);
}

TEST_F(ConfigTest, testMergeWorkspacesDefaultsToEmpty) {
  Config cfg = Config::withDefaults();
  EXPECT_TRUE(cfg.mergeWorkspaces.empty());
}

TEST_F(ConfigTest, testParseGeneratorValid) {
  EXPECT_EQ(Generator::UNIFORM, parseGenerator("uniform"));
  EXPECT_EQ(Generator::UNIFORM, parseGenerator("UNIFORM"));
  EXPECT_EQ(Generator::RANDOM, parseGenerator("random"));
  EXPECT_EQ(Generator::WEIGHTED, parseGenerator("weighted"));
}

TEST_F(ConfigTest, testParseGeneratorInvalid) {
  EXPECT_THROW(parseGenerator("invalid"), std::invalid_argument);
  EXPECT_THROW(parseGenerator(""), std::invalid_argument);
}

TEST_F(ConfigTest, testGeneratorToString) {
  EXPECT_EQ("uniform", generatorToString(Generator::UNIFORM));
  EXPECT_EQ("random", generatorToString(Generator::RANDOM));
  EXPECT_EQ("weighted", generatorToString(Generator::WEIGHTED));
}

TEST_F(ConfigTest, testStreamOperatorWithOutputDir) {
  Config cfg = Config::withDefaults();
  cfg.outputDir = "/tmp/output";
  cfg.timeout = 60;

  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();

  EXPECT_NE(std::string::npos, yaml.find("output-dir"));
  EXPECT_NE(std::string::npos, yaml.find("/tmp/output"));
  EXPECT_NE(std::string::npos, yaml.find("timeout: 60"));
}

TEST_F(ConfigTest, testStreamOperatorWithoutOptionals) {
  Config cfg = Config::withDefaults();
  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();

  EXPECT_EQ(std::string::npos, yaml.find("output-dir"));
  EXPECT_EQ(std::string::npos, yaml.find("timeout"));
}

TEST_F(ConfigTest, testStreamOperatorWithMutantsPerLine) {
  Config cfg = Config::withDefaults();
  cfg.mutantsPerLine = 3;
  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("mutants-per-line: 3"));
}

TEST_F(ConfigTest, testStreamOperatorDefaultMutantsPerLineOmitted) {
  Config cfg = Config::withDefaults();
  cfg.mutantsPerLine = 1;
  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();
  EXPECT_EQ(std::string::npos, yaml.find("mutants-per-line"));
}

TEST_F(ConfigTest, testStreamOperatorWithExtensionsAndPatterns) {
  Config cfg = Config::withDefaults();
  cfg.extensions = {"cpp", "h"};
  cfg.patterns = {"src/**", "!test/**"};
  cfg.operators = {"AOR", "ROR"};
  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("cpp"));
  EXPECT_NE(std::string::npos, yaml.find("src/**"));
  EXPECT_NE(std::string::npos, yaml.find("AOR"));
}

TEST_F(ConfigTest, testStreamOperatorWithLcovTracefiles) {
  Config cfg = Config::withDefaults();
  cfg.lcovTracefiles = {"/tmp/cov1.info", "/tmp/cov2.info"};
  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("/tmp/cov1.info"));
  EXPECT_NE(std::string::npos, yaml.find("/tmp/cov2.info"));
}

TEST_F(ConfigTest, testStreamOperatorWeightedGenerator) {
  Config cfg = Config::withDefaults();
  cfg.generator = Generator::WEIGHTED;
  std::ostringstream out;
  out << cfg;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("generator: weighted"));
}

TEST_F(ConfigTest, testParseGeneratorMixedCase) {
  EXPECT_EQ(Generator::UNIFORM, parseGenerator("Uniform"));
  EXPECT_EQ(Generator::RANDOM, parseGenerator("RANDOM"));
  EXPECT_EQ(Generator::WEIGHTED, parseGenerator("Weighted"));
}

}  // namespace sentinel
