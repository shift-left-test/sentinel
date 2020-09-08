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
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

class FilesystemTest : public ::testing::Test {
 protected:
  void SetUp() override {
    using util::filesystem::join;
    using util::filesystem::tempFilename;
    using util::filesystem::tempFilenameWithSuffix;
    using util::filesystem::tempPath;
    using util::filesystem::tempDirectory;

    BASE = tempDirectory("fixture");
    FILE = tempFilename(join(BASE, "file"));
    FILE_XML = tempFilenameWithSuffix(join(BASE, "file"), ".xml");
    DIRECTORY = tempDirectory(join(BASE, "dir"));
    COPY_DIRECTORY = tempDirectory(join(BASE, "dir"));
    NESTED_FILE = tempFilename(join(DIRECTORY, "file"));
    NESTED_FILE_TXT = tempFilenameWithSuffix(join(DIRECTORY, "file"), ".TXT");
    NESTED_DIRECTORY = tempDirectory(join(DIRECTORY, "dir"));
    UNKNOWN_PATH = "unknown/unknown";
    UNKNOWN_NESTED_PATH = tempPath(join(DIRECTORY, "unknown"));
  }

  void TearDown() override {
    util::filesystem::removeDirectories(BASE);
  }

  std::string BASE;
  std::string FILE;
  std::string FILE_XML;
  std::string NESTED_FILE;
  std::string NESTED_FILE_TXT;
  std::string DIRECTORY;
  std::string COPY_DIRECTORY;
  std::string NESTED_DIRECTORY;
  std::string UNKNOWN_PATH;
  std::string UNKNOWN_NESTED_PATH;
};

/*
static constexpr const char* FILE = "fixture/file";
static constexpr const char* NESTED_FILE = "fixture/dir/file";
static constexpr const char* DIRECTORY = "fixture/dir";
static constexpr const char* NESTED_DIRECTORY = "fixture/dir/dir";
static constexpr const char* UNKNOWN_PATH = "unknown/unknown";
static constexpr const char* UNKNOWN_NESTED_PATH = "fixture/dir/unknown";
*/

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
  auto temp1 = util::filesystem::tempPath("temp");
  auto temp2 = util::filesystem::join(temp1, "temp");

  util::filesystem::createDirectory(temp1);
  EXPECT_TRUE(util::filesystem::isDirectory(temp1));

  util::filesystem::removeDirectory(temp1);
  EXPECT_FALSE(util::filesystem::exists(temp1));

  util::filesystem::createDirectories(temp2);
  EXPECT_TRUE(util::filesystem::exists(temp2));

  util::filesystem::removeDirectories(temp2);
  EXPECT_FALSE(util::filesystem::exists(temp2));

  util::filesystem::removeDirectories(temp1);
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
  EXPECT_STREQ("a1/b1/c1", util::filesystem::join("a1", "b1", "c1").c_str());
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

TEST_F(FilesystemTest, testFindFilesInDirUsingRgx) {
  EXPECT_TRUE(util::filesystem::findFilesInDirUsingRgx(
      NESTED_DIRECTORY, std::regex(".+")).empty());

  auto filesInDir = util::filesystem::findFilesInDirUsingRgx(DIRECTORY,
      std::regex(".*\\.TXT"));
  EXPECT_EQ(filesInDir.size(), 1);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());

  filesInDir = util::filesystem::findFilesInDirUsingRgx(DIRECTORY,
      std::regex(".+"));
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE), filesInDir.end());
}

TEST_F(FilesystemTest, testFindFilesInDir) {
  EXPECT_TRUE(util::filesystem::findFilesInDir(
      NESTED_DIRECTORY).empty());

  auto filesInDir = util::filesystem::findFilesInDir(DIRECTORY);
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE), filesInDir.end());
}

TEST_F(FilesystemTest, testFindFilesInDirUsingExtention) {
  auto filesInDir = util::filesystem::findFilesInDirUsingExt(
     DIRECTORY, {});
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE), filesInDir.end());

  filesInDir = util::filesystem::findFilesInDirUsingExt(
     DIRECTORY, {"txt"});
  EXPECT_EQ(filesInDir.size(), 1);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());

  EXPECT_TRUE(util::filesystem::findFilesInDirUsingExt(
     DIRECTORY, {"xml"}).empty());

  filesInDir = util::filesystem::findFilesInDirUsingExt(
     BASE, {"txt", "xml"});
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), FILE_XML), filesInDir.end());
}

TEST_F(FilesystemTest, testCopyFile) {
  // Add content to file
  std::ofstream f(FILE);
  f << "some content\nother content\n";
  f.close();

  util::filesystem::copyFile(FILE, COPY_DIRECTORY);
  std::string copiedFilename = COPY_DIRECTORY + "/" + \
                               util::filesystem::filename(FILE);
  EXPECT_TRUE(util::filesystem::exists(copiedFilename));
  std::ifstream origFile(FILE);
  std::ifstream copiedFile(copiedFilename);
  std::string copiedFileLine, origFileLine;
  int lineCounter = 0;
  while (std::getline(copiedFile, copiedFileLine) &&
         std::getline(origFile, origFileLine)) {
    lineCounter += 1;
    EXPECT_EQ(copiedFileLine, origFileLine);
  }
  EXPECT_EQ(lineCounter, 2);
  origFile.close();
  copiedFile.close();

  EXPECT_THROW(util::filesystem::copyFile(COPY_DIRECTORY, COPY_DIRECTORY),
               IOException);
  EXPECT_THROW(util::filesystem::copyFile(UNKNOWN_PATH, COPY_DIRECTORY),
               IOException);
}

TEST_F(FilesystemTest, testComparePath) {
  EXPECT_TRUE(util::filesystem::comparePath(".", "../test"));
}

}  // namespace sentinel
