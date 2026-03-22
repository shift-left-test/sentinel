/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/Workspace.hpp"

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
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);
  writeFile(ws.getBackupDir() / "foo.cpp", "original content");

  ws.restoreBackup(srcRoot);

  EXPECT_TRUE(fs::exists(srcRoot / "foo.cpp"));
  EXPECT_FALSE(fs::exists(ws.getBackupDir() / "foo.cpp"));
}

TEST_F(EvaluationStageTest, testRestoreBackupEmptyBackupIsNoop) {
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);

  EXPECT_NO_THROW(ws.restoreBackup(srcRoot));
  EXPECT_TRUE(fs::is_empty(srcRoot));
}

TEST_F(EvaluationStageTest, testRestoreBackupNonexistentBackupIsNoop) {
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(srcRoot);

  EXPECT_NO_THROW(ws.restoreBackup(srcRoot));
  EXPECT_TRUE(fs::is_empty(srcRoot));
}
}  // namespace sentinel
