/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/YamlConfigWriter.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class YamlConfigWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = fs::temp_directory_path() / "SENTINEL_YAMLWRITER_TEST";
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

TEST_F(YamlConfigWriterTest, testWriteTemplateCreatesFile) {
  YamlConfigWriter::writeTemplate("sentinel.yaml");
  EXPECT_TRUE(fs::exists("sentinel.yaml"));
}

TEST_F(YamlConfigWriterTest, testWriteTemplateContentContainsVersionKey) {
  YamlConfigWriter::writeTemplate("sentinel.yaml");
  std::ifstream f("sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(f)), {});
  EXPECT_NE(content.find("version: 1"), std::string::npos);
}

TEST_F(YamlConfigWriterTest, testWriteTemplateThrowsWhenPathIsDirectory) {
  fs::create_directories(mBase / "sentinel.yaml");
  EXPECT_THROW(YamlConfigWriter::writeTemplate(mBase / "sentinel.yaml"), std::runtime_error);
}

TEST_F(YamlConfigWriterTest, testWriteTemplateOverwritesExistingFile) {
  {
    std::ofstream f("sentinel.yaml");
    f << "old content";
  }
  YamlConfigWriter::writeTemplate("sentinel.yaml");
  std::ifstream f("sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(f)), {});
  EXPECT_EQ(content.find("old content"), std::string::npos);
  EXPECT_NE(content.find("version: 1"), std::string::npos);
}
}  // namespace sentinel
