/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "helper/CaptureHelper.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationRunner.hpp"

namespace sentinel {

namespace fs = std::filesystem;

class MutationRunnerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Logger::clearCache();
    BASE = fs::temp_directory_path() / "SENTINEL_MUTRUNNERTEST_TMP_DIR";
    fs::remove_all(BASE);
    fs::create_directories(BASE);
  }

  void TearDown() override {
    fs::remove_all(BASE);
    Logger::clearCache();
  }

  void writeFile(const fs::path& p, const std::string& content) {
    std::ofstream f(p);
    ASSERT_TRUE(f.is_open()) << "Failed to open: " << p;
    f << content;
  }

  fs::path BASE;
};

TEST_F(MutationRunnerTest, testRestoreBackupCopiesFilesToSrcRoot) {
  auto backup = BASE / "backup";
  auto srcRoot = BASE / "src";
  fs::create_directories(backup);
  fs::create_directories(srcRoot);
  writeFile(backup / "foo.cpp", "original content");

  MutationRunner::restoreBackup(backup, srcRoot);

  EXPECT_TRUE(fs::exists(srcRoot / "foo.cpp"));
  EXPECT_FALSE(fs::exists(backup / "foo.cpp"));  // removed from backup after restore
}

TEST_F(MutationRunnerTest, testRestoreBackupEmptyBackupIsNoop) {
  auto backup = BASE / "backup";
  auto srcRoot = BASE / "src";
  fs::create_directories(backup);
  fs::create_directories(srcRoot);

  EXPECT_NO_THROW(MutationRunner::restoreBackup(backup, srcRoot));
  // srcRoot unchanged (still exists, no new files)
  EXPECT_TRUE(fs::is_empty(srcRoot));
}

TEST_F(MutationRunnerTest, testCopyTestReportToFiltersByExtension) {
  auto from = BASE / "from";
  auto to = BASE / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  MutationRunner::copyTestReportTo(from, to, {"xml"});

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_FALSE(fs::exists(to / "b.txt"));
}

TEST_F(MutationRunnerTest, testCopyTestReportToAllFilesWhenExtsEmpty) {
  auto from = BASE / "from";
  auto to = BASE / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  MutationRunner::copyTestReportTo(from, to, {});

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_TRUE(fs::exists(to / "b.txt"));
}

TEST_F(MutationRunnerTest, testCopyTestReportToCreatesDestDir) {
  auto from = BASE / "from";
  auto to = BASE / "to" / "nested";  // non-existent
  fs::create_directories(from);
  writeFile(from / "result.xml", "<xml/>");

  EXPECT_NO_THROW(MutationRunner::copyTestReportTo(from, to, {"xml"}));
  EXPECT_TRUE(fs::exists(to / "result.xml"));
}

TEST_F(MutationRunnerTest, testCopyTestReportToRecursive) {
  auto from = BASE / "from";
  auto sub = from / "sub";
  auto to = BASE / "to";
  fs::create_directories(sub);
  writeFile(from / "top.xml", "<top/>");
  writeFile(sub / "nested.xml", "<nested/>");  // unique filename

  MutationRunner::copyTestReportTo(from, to, {"xml"});

  EXPECT_TRUE(fs::exists(to / "top.xml"));
  EXPECT_TRUE(fs::exists(to / "nested.xml"));
}

TEST_F(MutationRunnerTest, testInitSetsDebugLevel) {
  Config cfg;
  cfg.debug = true;
  MutationRunner runner(cfg);
  runner.init();

  auto capture = CaptureHelper::getStdoutCapture();
  capture->capture();
  Logger::getLogger("sentinel")->debug("debug_marker");
  std::string out = capture->release();

  EXPECT_NE(out.find("debug_marker"), std::string::npos);
}

TEST_F(MutationRunnerTest, testInitSetsVerboseLevel) {
  Config cfg;
  cfg.verbose = true;
  MutationRunner runner(cfg);
  runner.init();

  auto capture = CaptureHelper::getStdoutCapture();
  capture->capture();
  Logger::getLogger("sentinel")->verbose("verbose_marker");
  std::string out = capture->release();

  EXPECT_NE(out.find("verbose_marker"), std::string::npos);
}

}  // namespace sentinel
