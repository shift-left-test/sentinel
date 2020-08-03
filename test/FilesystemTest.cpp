/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <gtest/gtest.h>
#include <string>
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

class FilesystemTest : public ::testing::Test {
 protected:
  static constexpr const char* FILE = "fixture/file";
  static constexpr const char* NESTED_FILE = "fixture/dir/file";
  static constexpr const char* DIRECTORY = "fixture/dir";
  static constexpr const char* NESTED_DIRECTORY = "fixture/dir/dir";
  static constexpr const char* UNKNOWN_PATH = "unknown";
  static constexpr const char* UNKNOWN_NESTED_PATH = "fixture/dir/unknown";
};

TEST_F(FilesystemTest, testExistsReturnTrueWhenValidFilesGiven) {
  EXPECT_TRUE(util::filesystem::exists(FILE));
  EXPECT_TRUE(util::filesystem::exists(NESTED_FILE));
  EXPECT_TRUE(util::filesystem::exists(DIRECTORY));
  EXPECT_TRUE(util::filesystem::exists(NESTED_DIRECTORY));
}

TEST_F(FilesystemTest, testExistsReturnFalseWhenInvalidFilesGiven) {
  EXPECT_FALSE(util::filesystem::exists(UNKNOWN_PATH));
  EXPECT_FALSE(util::filesystem::exists(UNKNOWN_NESTED_PATH));
}

TEST_F(FilesystemTest, testIsDirectoryReturnTrueWhenDirsGiven) {
  EXPECT_TRUE(util::filesystem::isDirectory(DIRECTORY));
  EXPECT_TRUE(util::filesystem::isDirectory(NESTED_DIRECTORY));
}

TEST_F(FilesystemTest, testIsDirectoryReturnFalseWhenInvalidDirsGiven) {
  EXPECT_FALSE(util::filesystem::isDirectory(FILE));
  EXPECT_FALSE(util::filesystem::isDirectory(NESTED_FILE));
  EXPECT_FALSE(util::filesystem::isDirectory(UNKNOWN_PATH));
  EXPECT_FALSE(util::filesystem::isDirectory(UNKNOWN_NESTED_PATH));
}

TEST_F(FilesystemTest, testIsRegularFileReturnTrueWhenValidFilesGiven) {
  EXPECT_TRUE(util::filesystem::isRegularFile(FILE));
  EXPECT_TRUE(util::filesystem::isRegularFile(NESTED_FILE));
}

TEST_F(FilesystemTest, testIsRegularFileReturnFalseWhenInvalidFilesGiven) {
  EXPECT_FALSE(util::filesystem::isRegularFile(DIRECTORY));
  EXPECT_FALSE(util::filesystem::isRegularFile(NESTED_DIRECTORY));
  EXPECT_FALSE(util::filesystem::isRegularFile(UNKNOWN_PATH));
  EXPECT_FALSE(util::filesystem::isRegularFile(UNKNOWN_NESTED_PATH));
}

TEST_F(FilesystemTest, testDirname) {
  EXPECT_STREQ(".", util::filesystem::dirname("").c_str());
  EXPECT_STREQ(".", util::filesystem::dirname(".").c_str());
  EXPECT_STREQ("/", util::filesystem::dirname("/").c_str());
  EXPECT_STREQ(".", util::filesystem::dirname("a").c_str());
  EXPECT_STREQ(".", util::filesystem::dirname("a/").c_str());
  EXPECT_STREQ("a", util::filesystem::dirname("a/b").c_str());
  EXPECT_STREQ("a", util::filesystem::dirname("a/b/").c_str());
  EXPECT_STREQ("a/b", util::filesystem::dirname("a/b/.").c_str());
  EXPECT_STREQ("a/b", util::filesystem::dirname("a/b/c.file").c_str());
  EXPECT_STREQ("a/b", util::filesystem::dirname("a/b/c.file/").c_str());
}

TEST_F(FilesystemTest, testFilename) {
  EXPECT_STREQ("", util::filesystem::filename("").c_str());
  EXPECT_STREQ("", util::filesystem::filename("/").c_str());
  EXPECT_STREQ("", util::filesystem::filename("//").c_str());
  EXPECT_STREQ("file", util::filesystem::filename("a/b/file").c_str());
  EXPECT_STREQ("file", util::filesystem::filename(".././file").c_str());
  EXPECT_STREQ("file", util::filesystem::filename("a/b/file/").c_str());
}

}  // namespace sentinel
