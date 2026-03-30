/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {
namespace fs = std::filesystem;

class EvaluationStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_EVALSTAGE_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
  }
  void TearDown() override {
    fs::remove_all(mBase);
  }
  fs::path mBase;
};

TEST_F(EvaluationStageTest, testRestoreBackupCopiesFilesToSrcRoot) {
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);
  testutil::writeFile(ws.getBackupDir() / "foo.cpp", "original content");

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

TEST_F(EvaluationStageTest, testRestoreBackupPreservesSubdirectoryStructure) {
  Workspace ws(mBase);
  const auto srcRoot = mBase / "src";
  const auto deepBackup = ws.getBackupDir() / "sub" / "dir";
  fs::create_directories(deepBackup);
  fs::create_directories(srcRoot);
  testutil::writeFile(deepBackup / "deep.cpp", "deep content");

  ws.restoreBackup(srcRoot);

  EXPECT_TRUE(fs::exists(srcRoot / "sub" / "dir" / "deep.cpp"));
  EXPECT_FALSE(fs::exists(ws.getBackupDir() / "sub" / "dir" / "deep.cpp"));
}

TEST_F(EvaluationStageTest, testRestoreBackupOverwritesExisting) {
  Workspace ws(mBase);
  const auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);
  testutil::writeFile(srcRoot / "foo.cpp", "modified");
  testutil::writeFile(ws.getBackupDir() / "foo.cpp", "original");

  ws.restoreBackup(srcRoot);

  EXPECT_EQ("original", testutil::readFile(srcRoot / "foo.cpp"));
}

}  // namespace sentinel
