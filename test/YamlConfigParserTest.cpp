/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
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
}  // namespace sentinel
