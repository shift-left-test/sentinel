/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/io.hpp"

namespace fs = std::filesystem;

namespace sentinel {

using ::testing::HasSubstr;

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

TEST_F(IoTest, testSyncFilesCopiesXmlOnly) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.txt", "text");

  io::syncFiles(from, to);

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_FALSE(fs::exists(to / "b.txt"));
}

TEST_F(IoTest, testSyncFilesCopiesXmlCaseInsensitive) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  writeFile(from / "a.xml", "<xml/>");
  writeFile(from / "b.XML", "<XML/>");
  writeFile(from / "c.txt", "text");

  io::syncFiles(from, to);

  EXPECT_TRUE(fs::exists(to / "a.xml"));
  EXPECT_TRUE(fs::exists(to / "b.XML"));
  EXPECT_FALSE(fs::exists(to / "c.txt"));
}

TEST_F(IoTest, testSyncFilesClearsDestinationFirst) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  fs::create_directories(to);
  writeFile(to / "old.xml", "old");
  writeFile(from / "new.xml", "<xml/>");

  io::syncFiles(from, to);

  EXPECT_FALSE(fs::exists(to / "old.xml"));
  EXPECT_TRUE(fs::exists(to / "new.xml"));
}

TEST_F(IoTest, testSyncFilesDuplicateNameInSubdirsThrows) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  writeFile(from / "sub1" / "result.xml", "<xml>1</xml>");
  writeFile(from / "sub2" / "result.xml", "<xml>2</xml>");

  // flat copy: second file with same name should cause filesystem_error
  EXPECT_THROW(io::syncFiles(from, to), std::filesystem::filesystem_error);
}

TEST_F(IoTest, testSyncFilesNonXmlOnlyResultsInEmptyDestination) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);
  writeFile(from / "result.json", "{}");
  writeFile(from / "data.txt", "text");
  writeFile(from / "report.html", "<html/>");

  io::syncFiles(from, to);

  EXPECT_TRUE(fs::is_directory(to));
  EXPECT_TRUE(fs::is_empty(to));
}

TEST_F(IoTest, testSyncFilesEmptySourceResultsInEmptyDestination) {
  auto from = mTestDir / "from";
  auto to = mTestDir / "to";
  fs::create_directories(from);

  io::syncFiles(from, to);

  EXPECT_TRUE(fs::is_directory(to));
  EXPECT_TRUE(fs::is_empty(to));
}

TEST_F(IoTest, testReadLastLinesReturnsLastNLines) {
  auto file = mTestDir / "test.log";
  writeFile(file, "line1\nline2\nline3\nline4\nline5\n");

  auto result = io::readLastLines(file, 3);
  EXPECT_EQ(result, "line3\nline4\nline5");
}

TEST_F(IoTest, testReadLastLinesFewerLinesThanRequested) {
  auto file = mTestDir / "test.log";
  writeFile(file, "only\ntwo\n");

  auto result = io::readLastLines(file, 10);
  EXPECT_EQ(result, "only\ntwo");
}

TEST_F(IoTest, testReadLastLinesEmptyFile) {
  auto file = mTestDir / "test.log";
  writeFile(file, "");

  auto result = io::readLastLines(file, 5);
  EXPECT_EQ(result, "");
}

TEST_F(IoTest, testReadLastLinesNonexistentFile) {
  auto result = io::readLastLines("/nonexistent/file.log", 5);
  EXPECT_EQ(result, "");
}

TEST_F(IoTest, testReadLastLinesZeroReturnsEmpty) {
  auto file = mTestDir / "test.log";
  writeFile(file, "line1\nline2\nline3\n");

  auto result = io::readLastLines(file, 0);
  EXPECT_EQ(result, "");
}

TEST_F(IoTest, testReadLastLinesOneReturnsLastLine) {
  auto file = mTestDir / "test.log";
  writeFile(file, "line1\nline2\nline3\n");

  auto result = io::readLastLines(file, 1);
  EXPECT_EQ(result, "line3");
}

TEST_F(IoTest, testAppendLogTailAppendsLastLinesWithSeparators) {
  auto file = mTestDir / "test.log";
  writeFile(file, "line1\nline2\nline3\n");

  std::string msg = "Error occurred";
  io::appendLogTail(&msg, file, 2, "build output");
  std::string expected =
      "Error occurred\n\n"
      "       --- build output (last 2 lines) ---\n"
      "       line2\n"
      "       line3\n"
      "       -----------------------------------";
  EXPECT_EQ(msg, expected);
}

TEST_F(IoTest, testAppendLogTailDefaultLabel) {
  auto file = mTestDir / "test.log";
  writeFile(file, "line1\nline2\nline3\n");

  std::string msg = "Error occurred";
  io::appendLogTail(&msg, file, 2);
  EXPECT_THAT(msg, HasSubstr("--- output (last 2 lines) ---"));
}

TEST_F(IoTest, testAppendLogTailDoesNothingForEmptyFile) {
  auto file = mTestDir / "empty.log";
  writeFile(file, "");

  std::string msg = "Error occurred";
  io::appendLogTail(&msg, file, 5);
  EXPECT_EQ(msg, "Error occurred");
}

TEST_F(IoTest, testAppendLogTailDoesNothingForNonexistentFile) {
  std::string msg = "Error occurred";
  io::appendLogTail(&msg, mTestDir / "nonexistent.log", 5);
  EXPECT_EQ(msg, "Error occurred");
}

}  // namespace sentinel
