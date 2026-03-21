/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "sentinel/stages/BaselineTestStage.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class BaselineTestStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = fs::temp_directory_path() / "SENTINEL_BASELINETEST_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
  }
  void TearDown() override { fs::remove_all(mBase); }
  void writeFile(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p);
    f << content;
  }
  fs::path mBase;
};

TEST_F(BaselineTestStageTest, testCopyTestReportToFiltersByExtension) {
  auto from = mBase / "from";
  auto to = mBase / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  BaselineTestStage::copyTestReportTo(from, to, {"xml"});

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_FALSE(fs::exists(to / "b.txt"));
}

TEST_F(BaselineTestStageTest, testCopyTestReportToAllFilesWhenExtsEmpty) {
  auto from = mBase / "from";
  auto to = mBase / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  BaselineTestStage::copyTestReportTo(from, to, {});

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_TRUE(fs::exists(to / "b.txt"));
}

TEST_F(BaselineTestStageTest, testCopyTestReportToClearsDestinationFirst) {
  auto from = mBase / "from";
  auto to = mBase / "to";
  fs::create_directories(from);
  fs::create_directories(to);
  writeFile(to / "old.xml", "old");
  writeFile(from / "new.xml", "<xml/>");

  BaselineTestStage::copyTestReportTo(from, to, {"xml"});

  EXPECT_FALSE(fs::exists(to / "old.xml"));
  EXPECT_TRUE(fs::exists(to / "new.xml"));
}
}  // namespace sentinel
