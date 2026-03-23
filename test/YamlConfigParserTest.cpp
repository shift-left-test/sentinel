/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/YamlConfigParser.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class YamlConfigParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = fs::temp_directory_path() / "SENTINEL_YAMLPARSER_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mOrigCwd = fs::current_path();
    fs::current_path(mBase);
  }
  void TearDown() override {
    fs::current_path(mOrigCwd);
    fs::remove_all(mBase);
  }
  fs::path mBase;
  fs::path mOrigCwd;
};

TEST_F(YamlConfigParserTest, testWriteTemplateCreatesFile) {
  YamlConfigParser::writeTemplate("sentinel.yaml");
  EXPECT_TRUE(fs::exists("sentinel.yaml"));
}

TEST_F(YamlConfigParserTest, testWriteTemplateContentContainsVersionKey) {
  YamlConfigParser::writeTemplate("sentinel.yaml");
  std::ifstream f("sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(f)), {});
  EXPECT_NE(content.find("version: 1"), std::string::npos);
}

TEST_F(YamlConfigParserTest, testWriteTemplateThrowsWhenPathIsDirectory) {
  fs::create_directories(mBase / "sentinel.yaml");
  EXPECT_THROW(YamlConfigParser::writeTemplate(mBase / "sentinel.yaml"), std::runtime_error);
}

TEST_F(YamlConfigParserTest, testWriteTemplateOverwritesExistingFile) {
  {
    std::ofstream f("sentinel.yaml");
    f << "old content";
  }
  YamlConfigParser::writeTemplate("sentinel.yaml");
  std::ifstream f("sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(f)), {});
  EXPECT_EQ(content.find("old content"), std::string::npos);
  EXPECT_NE(content.find("version: 1"), std::string::npos);
}
}  // namespace sentinel
