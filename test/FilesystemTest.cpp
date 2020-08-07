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

#include <fstream>
#include <iostream>
#include <string>

#include "sentinel/util/filesystem.hpp"


namespace sentinel {

class FilesystemTest : public ::testing::Test {
 protected:
  static constexpr const char* FILE = "fixture/file";
  static constexpr const char* NESTED_FILE = "fixture/dir/file";
  static constexpr const char* DIRECTORY = "fixture/dir";
  static constexpr const char* NESTED_DIRECTORY = "fixture/dir/dir";
  static constexpr const char* UNKNOWN_PATH = "unknown/unknown";
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

TEST_F(FilesystemTest, testCreateAndRemoveDirectoryWhenValidDirsGiven) {
  util::filesystem::createDirectory("fixture/temp");
  EXPECT_TRUE(util::filesystem::isDirectory("fixture/temp"));

  util::filesystem::removeDirectory("fixture/temp");
  EXPECT_FALSE(util::filesystem::exists("fixture/temp"));

  util::filesystem::createDirectories("fixture/temps/temp");
  std::ofstream tempfile1("fixture/temps/temp1.txt");
  tempfile1.close();
  std::ofstream tempfile2("fixture/temps/temp/temp2.txt");
  tempfile1.close();
  EXPECT_TRUE(util::filesystem::isDirectory("fixture/temps/temp"));

  util::filesystem::removeDirectories("fixture/temps");
  EXPECT_FALSE(util::filesystem::exists("fixture/temps"));
}

TEST_F(FilesystemTest, testCreateAndRemoveDirectoryWhenInvalidDirsGiven) {
  EXPECT_THROW(util::filesystem::createDirectory(DIRECTORY), IOException);
  EXPECT_THROW(util::filesystem::createDirectories(NESTED_DIRECTORY),
               IOException);
  EXPECT_THROW(util::filesystem::createDirectory("."), IOException);
  EXPECT_THROW(util::filesystem::createDirectory(".."), IOException);
  EXPECT_THROW(util::filesystem::createDirectory("//"), IOException);
}

TEST_F(FilesystemTest, testTempPath) {
  std::string tempname1 = util::filesystem::tempPath("temp");
  std::string tempname2 = util::filesystem::tempPath("temp");
  EXPECT_NE(tempname1.compare(tempname2), 0);
}

TEST_F(FilesystemTest, testTempDirectory) {
  std::string tempname1 = util::filesystem::tempDirectory("");
  std::string tempname2 = util::filesystem::tempDirectory("");

  EXPECT_NE(tempname1.compare(tempname2), 0);
  EXPECT_TRUE(util::filesystem::isDirectory(tempname1));
  EXPECT_TRUE(util::filesystem::isDirectory(tempname2));

  util::filesystem::removeDirectory(tempname1);
  util::filesystem::removeDirectory(tempname2);
}

TEST_F(FilesystemTest, testJoinPath) {
  EXPECT_STREQ("a", util::filesystem::join("a").c_str());
  EXPECT_STREQ("a/b", util::filesystem::join("a", "b").c_str());
  EXPECT_STREQ("a/b/c", util::filesystem::join("a", "b", "c").c_str());
}

TEST_F(FilesystemTest, testRenameWorksWhenBothFilesExist) {
  std::string src_filename = util::filesystem::tempPath("");
  std::string dest_filename = util::filesystem::tempPath("");
  std::ofstream(src_filename.c_str()).put('a');
  std::ofstream(dest_filename.c_str()).put('b');
  util::filesystem::rename(src_filename, dest_filename);
  std::ifstream dest_file(dest_filename.c_str());
  std::string content((std::istreambuf_iterator<char>(dest_file)),
                       std::istreambuf_iterator<char>());

  EXPECT_FALSE(util::filesystem::exists(src_filename));
  EXPECT_TRUE(util::filesystem::isRegularFile(dest_filename));
  EXPECT_EQ(content.compare("a"), 0);

  util::filesystem::removeFile(dest_filename);
}

TEST_F(FilesystemTest, testRenameWorksWhenOnlySourceFileExists) {
  std::string src_filename = util::filesystem::tempPath("");
  std::string dest_filename = util::filesystem::tempPath("");
  std::ofstream(src_filename.c_str()).put('a');
  util::filesystem::rename(src_filename, dest_filename);
  std::ifstream dest_file(dest_filename.c_str());
  std::string content((std::istreambuf_iterator<char>(dest_file)),
                       std::istreambuf_iterator<char>());

  EXPECT_FALSE(util::filesystem::exists(src_filename));
  EXPECT_TRUE(util::filesystem::isRegularFile(dest_filename));
  EXPECT_EQ(content.compare("a"), 0);

  util::filesystem::removeFile(dest_filename);
}

TEST_F(FilesystemTest, testReplaceFileFailsWhenInvalidPathsGiven) {
  std::string src_filename = util::filesystem::tempPath("");
  std::ofstream(src_filename.c_str()).put('a');

  EXPECT_THROW(util::filesystem::rename(UNKNOWN_PATH, UNKNOWN_PATH),
               IOException);
  EXPECT_THROW(util::filesystem::rename(src_filename, UNKNOWN_PATH),
               IOException);
  EXPECT_THROW(util::filesystem::rename(src_filename, DIRECTORY),
               IOException);

  util::filesystem::removeFile(src_filename);
}

}  // namespace sentinel
