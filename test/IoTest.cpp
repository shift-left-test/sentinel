/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/io.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class IoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mTestDir = fs::temp_directory_path() / "SENTINEL_IOTEST_TMP_DIR";
    fs::remove_all(mTestDir);
  }

  void TearDown() override {
    fs::remove_all(mTestDir);
  }

  fs::path mTestDir;
};

TEST_F(IoTest, testEnsureDirectoryExistsCreatesNewDirectory) {
  EXPECT_FALSE(fs::exists(mTestDir));
  io::ensureDirectoryExists(mTestDir);
  EXPECT_TRUE(fs::is_directory(mTestDir));
}

TEST_F(IoTest, testEnsureDirectoryExistsCreatesNestedDirectories) {
  auto nested = mTestDir / "a" / "b" / "c";
  EXPECT_FALSE(fs::exists(nested));
  io::ensureDirectoryExists(nested);
  EXPECT_TRUE(fs::is_directory(nested));
}

TEST_F(IoTest, testEnsureDirectoryExistsSucceedsWhenDirectoryAlreadyExists) {
  fs::create_directories(mTestDir);
  EXPECT_TRUE(fs::is_directory(mTestDir));
  EXPECT_NO_THROW(io::ensureDirectoryExists(mTestDir));
  EXPECT_TRUE(fs::is_directory(mTestDir));
}

TEST_F(IoTest, testEnsureDirectoryExistsThrowsWhenPathIsAFile) {
  fs::create_directories(mTestDir);
  auto filePath = mTestDir / "somefile";
  std::ofstream(filePath).close();
  EXPECT_TRUE(fs::exists(filePath));
  EXPECT_FALSE(fs::is_directory(filePath));
  EXPECT_THROW(io::ensureDirectoryExists(filePath), InvalidArgumentException);
}

}  // namespace sentinel
