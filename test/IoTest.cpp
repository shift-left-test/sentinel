/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/io.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class IoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mTestDir = testTempDir("SENTINEL_IOTEST_TMP_DIR");
    fs::remove_all(mTestDir);
  }

  void TearDown() override {
    fs::remove_all(mTestDir);
  }

  void writeFile(const fs::path& p, const std::string& content) {
    testutil::writeFile(p, content);
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

TEST_F(IoTest, testSyncFilesFiltersByExtension) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  io::syncFiles(from, to, {"xml"});

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_FALSE(fs::exists(to / "b.txt"));
}

TEST_F(IoTest, testSyncFilesAllFilesWhenExtsEmpty) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  io::syncFiles(from, to, {});

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_TRUE(fs::exists(to / "b.txt"));
}

TEST_F(IoTest, testSyncFilesClearsDestinationFirst) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  fs::create_directories(to);
  writeFile(to / "old.xml", "old");
  writeFile(from / "new.xml", "<xml/>");

  io::syncFiles(from, to, {"xml"});

  EXPECT_FALSE(fs::exists(to / "old.xml"));
  EXPECT_TRUE(fs::exists(to / "new.xml"));
}

TEST_F(IoTest, testSyncFilesOverloadCopiesAllFiles) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  io::syncFiles(from, to);

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_TRUE(fs::exists(to / "b.txt"));
}

}  // namespace sentinel
