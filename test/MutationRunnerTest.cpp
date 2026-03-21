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

// Verify the partition slice formula used in MutationRunner is correct.
TEST_F(MutationRunnerTest, testPartitionSliceFormula) {
  // Simulates: partIdx=N, partCount=TOTAL applied to a list of 'size' items
  auto applySlice = [](std::size_t size, std::size_t partIdx, std::size_t partCount)
      -> std::pair<std::size_t, std::size_t> {
    std::size_t start = (partIdx - 1) * size / partCount;
    std::size_t end   = partIdx * size / partCount;
    return {start, end};
  };

  // 10 mutants split into 2 partitions
  EXPECT_EQ(std::make_pair(0UL, 5UL), applySlice(10, 1, 2));
  EXPECT_EQ(std::make_pair(5UL, 10UL), applySlice(10, 2, 2));

  // 10 mutants split into 3 partitions (integer division)
  EXPECT_EQ(std::make_pair(0UL, 3UL), applySlice(10, 1, 3));
  EXPECT_EQ(std::make_pair(3UL, 6UL), applySlice(10, 2, 3));
  EXPECT_EQ(std::make_pair(6UL, 10UL), applySlice(10, 3, 3));

  // Edge: single mutant, single partition
  EXPECT_EQ(std::make_pair(0UL, 1UL), applySlice(1, 1, 1));

  // Edge: 0 mutants
  EXPECT_EQ(std::make_pair(0UL, 0UL), applySlice(0, 1, 2));

  // Invariant: union of all slices covers [0, size) with no gaps or overlaps
  for (std::size_t partCount : {2UL, 3UL, 4UL, 7UL}) {
    constexpr std::size_t kSize = 10;
    std::size_t prev = 0;
    for (std::size_t i = 1; i <= partCount; i++) {
      auto [s, e] = applySlice(kSize, i, partCount);
      EXPECT_EQ(s, prev);
      prev = e;
    }
    EXPECT_EQ(prev, kSize);
  }
}

}  // namespace sentinel
