/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/stages/EvaluationStage.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class EvaluationStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = fs::temp_directory_path() / "SENTINEL_EVALSTAGE_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
  }
  void TearDown() override { fs::remove_all(mBase); }
  void writeFile(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p); f << content;
  }
  fs::path mBase;
};

TEST_F(EvaluationStageTest, testRestoreBackupCopiesFilesToSrcRoot) {
  auto backup = mBase / "backup";
  auto srcRoot = mBase / "src";
  fs::create_directories(backup);
  fs::create_directories(srcRoot);
  writeFile(backup / "foo.cpp", "original content");

  EvaluationStage::restoreBackup(backup, srcRoot);

  EXPECT_TRUE(fs::exists(srcRoot / "foo.cpp"));
  EXPECT_FALSE(fs::exists(backup / "foo.cpp"));
}

TEST_F(EvaluationStageTest, testRestoreBackupEmptyBackupIsNoop) {
  auto backup = mBase / "backup";
  auto srcRoot = mBase / "src";
  fs::create_directories(backup);
  fs::create_directories(srcRoot);

  EXPECT_NO_THROW(EvaluationStage::restoreBackup(backup, srcRoot));
  EXPECT_TRUE(fs::is_empty(srcRoot));
}
}  // namespace sentinel
