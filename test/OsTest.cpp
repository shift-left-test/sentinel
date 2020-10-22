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
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

class OsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    using os::path::join;
    using os::tempFilename;
    using os::tempFilename;
    using os::tempPath;
    using os::tempDirectory;

    BASE = tempDirectory("fixture");
    FILE = tempFilename(join(BASE, "file"));
    FILE_XML = tempFilename(join(BASE, "file"), ".xml");
    DIRECTORY = tempDirectory(join(BASE, "dir"));
    COPY_DIRECTORY = tempDirectory(join(BASE, "dir"));
    NESTED_FILE = tempFilename(join(DIRECTORY, "file"));
    NESTED_FILE_TXT = tempFilename(join(DIRECTORY, "file"), ".TXT");
    NESTED_DIRECTORY = tempDirectory(join(DIRECTORY, "dir"));
    UNKNOWN_PATH = "unknown/unknown";
    UNKNOWN_NESTED_PATH = tempPath(join(DIRECTORY, "unknown"));
  }

  void TearDown() override {
    os::removeDirectories(BASE);
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

TEST_F(OsTest, testExistsReturnTrueWhenValidFilesGiven) {
  EXPECT_TRUE(os::path::exists(FILE));
  EXPECT_TRUE(os::path::exists(NESTED_FILE));
  EXPECT_TRUE(os::path::exists(DIRECTORY));
  EXPECT_TRUE(os::path::exists(NESTED_DIRECTORY));
}

TEST_F(OsTest, testExistsReturnFalseWhenInvalidFilesGiven) {
  EXPECT_FALSE(os::path::exists(UNKNOWN_PATH));
  EXPECT_FALSE(os::path::exists(UNKNOWN_NESTED_PATH));
}

TEST_F(OsTest, testIsDirectoryReturnTrueWhenDirsGiven) {
  EXPECT_TRUE(os::path::isDirectory(DIRECTORY));
  EXPECT_TRUE(os::path::isDirectory(NESTED_DIRECTORY));
}

TEST_F(OsTest, testIsDirectoryReturnFalseWhenInvalidDirsGiven) {
  EXPECT_FALSE(os::path::isDirectory(FILE));
  EXPECT_FALSE(os::path::isDirectory(NESTED_FILE));
  EXPECT_FALSE(os::path::isDirectory(UNKNOWN_PATH));
  EXPECT_FALSE(os::path::isDirectory(UNKNOWN_NESTED_PATH));
}

TEST_F(OsTest, testIsRegularFileReturnTrueWhenValidFilesGiven) {
  EXPECT_TRUE(os::path::isRegularFile(FILE));
  EXPECT_TRUE(os::path::isRegularFile(NESTED_FILE));
}

TEST_F(OsTest, testIsRegularFileReturnFalseWhenInvalidFilesGiven) {
  EXPECT_FALSE(os::path::isRegularFile(DIRECTORY));
  EXPECT_FALSE(os::path::isRegularFile(NESTED_DIRECTORY));
  EXPECT_FALSE(os::path::isRegularFile(UNKNOWN_PATH));
  EXPECT_FALSE(os::path::isRegularFile(UNKNOWN_NESTED_PATH));
}

TEST_F(OsTest, testDirname) {
  EXPECT_STREQ(".", os::path::dirname("").c_str());
  EXPECT_STREQ(".", os::path::dirname(".").c_str());
  EXPECT_STREQ("/", os::path::dirname("/").c_str());
  EXPECT_STREQ(".", os::path::dirname("a").c_str());
  EXPECT_STREQ(".", os::path::dirname("a/").c_str());
  EXPECT_STREQ("a", os::path::dirname("a/b").c_str());
  EXPECT_STREQ("a", os::path::dirname("a/b/").c_str());
  EXPECT_STREQ("a/b", os::path::dirname("a/b/.").c_str());
  EXPECT_STREQ("a/b", os::path::dirname("a/b/c.file").c_str());
  EXPECT_STREQ("a/b", os::path::dirname("a/b/c.file/").c_str());
}

TEST_F(OsTest, testFilename) {
  EXPECT_STREQ("", os::path::filename("").c_str());
  EXPECT_STREQ("", os::path::filename("/").c_str());
  EXPECT_STREQ("", os::path::filename("//").c_str());
  EXPECT_STREQ("file", os::path::filename("a/b/file").c_str());
  EXPECT_STREQ("file", os::path::filename(".././file").c_str());
  EXPECT_STREQ("file", os::path::filename("a/b/file/").c_str());
}

TEST_F(OsTest, testCreateAndRemoveDirectoryWhenValidDirsGiven) {
  auto temp1 = os::tempPath("temp");
  auto temp2 = os::path::join(temp1, "temp");

  os::createDirectory(temp1);
  EXPECT_TRUE(os::path::isDirectory(temp1));

  os::removeDirectory(temp1);
  EXPECT_FALSE(os::path::exists(temp1));

  os::createDirectories(temp2);
  EXPECT_TRUE(os::path::exists(temp2));

  os::removeDirectories(temp2);
  EXPECT_FALSE(os::path::exists(temp2));

  os::removeDirectories(temp1);
}

TEST_F(OsTest, testRemoveDirectoryWhenInvalidDirGiven) {
  EXPECT_THROW(os::removeDirectory("."), IOException);
  EXPECT_THROW(os::removeDirectory(".."), IOException);
  EXPECT_THROW(os::removeDirectory(BASE), IOException);
  EXPECT_THROW(os::removeDirectory(UNKNOWN_PATH), IOException);
}

TEST_F(OsTest, testRemoveFileFailWhenInvalidFileGiven) {
  EXPECT_THROW(os::removeFile("."), IOException);
  EXPECT_THROW(os::removeFile(".."), IOException);
  EXPECT_THROW(os::removeFile(BASE), IOException);
  EXPECT_THROW(os::removeFile(UNKNOWN_PATH), IOException);
}

TEST_F(OsTest, testCreateAndRemoveDirectoryWhenInvalidDirsGiven) {
  EXPECT_THROW(os::createDirectory(DIRECTORY), IOException);
  EXPECT_THROW(os::createDirectories(NESTED_DIRECTORY),
               IOException);
  EXPECT_THROW(os::createDirectory("."), IOException);
  EXPECT_THROW(os::createDirectory(".."), IOException);
  EXPECT_THROW(os::createDirectory("//"), IOException);
}

TEST_F(OsTest, testTempPath) {
  std::string tempname1 = os::tempPath("temp");
  std::string tempname2 = os::tempPath("temp");
  EXPECT_NE(tempname1.compare(tempname2), 0);
}

TEST_F(OsTest, testTempDirectory) {
  std::string tempname1 = os::tempDirectory("");
  std::string tempname2 = os::tempDirectory("");

  EXPECT_NE(tempname1.compare(tempname2), 0);
  EXPECT_TRUE(os::path::isDirectory(tempname1));
  EXPECT_TRUE(os::path::isDirectory(tempname2));

  os::removeDirectory(tempname1);
  os::removeDirectory(tempname2);

  EXPECT_THROW(os::tempDirectory(UNKNOWN_PATH), IOException);
}

TEST_F(OsTest, testJoinPath) {
  EXPECT_STREQ("a", os::path::join("a").c_str());
  EXPECT_STREQ("a/b", os::path::join("a", "b").c_str());
  EXPECT_STREQ("a1/b1/c1", os::path::join("a1", "b1", "c1").c_str());
}

TEST_F(OsTest, testRenameWorksWhenBothFilesExist) {
  std::string src_filename = os::tempPath("");
  std::string dest_filename = os::tempPath("");
  std::ofstream(src_filename.c_str()).put('a');
  std::ofstream(dest_filename.c_str()).put('b');
  os::rename(src_filename, dest_filename);
  std::ifstream dest_file(dest_filename.c_str());
  std::string content((std::istreambuf_iterator<char>(dest_file)),
                       std::istreambuf_iterator<char>());

  EXPECT_FALSE(os::path::exists(src_filename));
  EXPECT_TRUE(os::path::isRegularFile(dest_filename));
  EXPECT_EQ(content.compare("a"), 0);

  os::removeFile(dest_filename);
}

TEST_F(OsTest, testRenameWorksWhenOnlySourceFileExists) {
  std::string src_filename = os::tempPath("");
  std::string dest_filename = os::tempPath("");
  std::ofstream(src_filename.c_str()).put('a');
  os::rename(src_filename, dest_filename);
  std::ifstream dest_file(dest_filename.c_str());
  std::string content((std::istreambuf_iterator<char>(dest_file)),
                       std::istreambuf_iterator<char>());

  EXPECT_FALSE(os::path::exists(src_filename));
  EXPECT_TRUE(os::path::isRegularFile(dest_filename));
  EXPECT_EQ(content.compare("a"), 0);

  os::removeFile(dest_filename);
}

TEST_F(OsTest, testReplaceFileFailsWhenInvalidPathsGiven) {
  std::string src_filename = os::tempPath("");
  std::ofstream(src_filename.c_str()).put('a');

  EXPECT_THROW(os::rename(UNKNOWN_PATH, UNKNOWN_PATH),
               IOException);
  EXPECT_THROW(os::rename(src_filename, UNKNOWN_PATH),
               IOException);
  EXPECT_THROW(os::rename(src_filename, DIRECTORY),
               IOException);

  os::removeFile(src_filename);
}

TEST_F(OsTest, testFindFilesInDirUsingRgx) {
  EXPECT_TRUE(os::findFilesInDirUsingRgx(
      NESTED_DIRECTORY, std::regex(".+")).empty());

  auto filesInDir = os::findFilesInDirUsingRgx(DIRECTORY,
      std::regex(".*\\.TXT"));
  EXPECT_EQ(filesInDir.size(), 1);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());

  filesInDir = os::findFilesInDirUsingRgx(DIRECTORY,
      std::regex(".+"));
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE), filesInDir.end());
  EXPECT_THROW(os::findFilesInDirUsingRgx(UNKNOWN_PATH,
                                          std::regex(".+")).empty(),
               IOException);
}

TEST_F(OsTest, testFindFilesInDir) {
  EXPECT_TRUE(os::findFilesInDir(
      NESTED_DIRECTORY).empty());

  auto filesInDir = os::findFilesInDir(DIRECTORY);
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE), filesInDir.end());
}

TEST_F(OsTest, testFindFilesInDirUsingExtention) {
  auto filesInDir = os::findFilesInDirUsingExt(
     DIRECTORY, {});
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE), filesInDir.end());

  filesInDir = os::findFilesInDirUsingExt(
     DIRECTORY, {"txt"});
  EXPECT_EQ(filesInDir.size(), 1);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());

  EXPECT_TRUE(os::findFilesInDirUsingExt(
     DIRECTORY, {"xml"}).empty());

  filesInDir = os::findFilesInDirUsingExt(
     BASE, {"txt", "xml"});
  EXPECT_EQ(filesInDir.size(), 2);
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), NESTED_FILE_TXT), filesInDir.end());
  EXPECT_NE(std::find(filesInDir.begin(),
      filesInDir.end(), FILE_XML), filesInDir.end());
}

TEST_F(OsTest, testCopyFile) {
  // Add content to file
  std::ofstream f(FILE);
  f << "some content\nother content\n";
  f.close();

  os::copyFile(FILE, COPY_DIRECTORY);
  std::string copiedFilename = COPY_DIRECTORY + "/" + \
                               os::path::filename(FILE);
  EXPECT_TRUE(os::path::exists(copiedFilename));
  std::ifstream origFile(FILE);
  std::ifstream copiedFile(copiedFilename);
  std::string copiedFileLine, origFileLine;
  std::size_t lineCounter = 0;
  while (std::getline(copiedFile, copiedFileLine) &&
         std::getline(origFile, origFileLine)) {
    lineCounter += 1;
    EXPECT_EQ(copiedFileLine, origFileLine);
  }
  EXPECT_EQ(lineCounter, 2);
  origFile.close();
  copiedFile.close();

  EXPECT_THROW(os::copyFile(COPY_DIRECTORY, COPY_DIRECTORY),
               IOException);
  EXPECT_THROW(os::copyFile(UNKNOWN_PATH, COPY_DIRECTORY),
               IOException);
}

TEST_F(OsTest, testComparePath) {
  EXPECT_TRUE(os::path::comparePath(".", "../test"));
  EXPECT_THROW(os::path::comparePath(UNKNOWN_PATH, UNKNOWN_PATH), IOException);
}

TEST_F(OsTest, testRelativePath) {
  EXPECT_EQ(os::path::getRelativePath(BASE, BASE), ".");
  EXPECT_EQ(os::path::getRelativePath(FILE, BASE), os::path::filename(FILE));
  EXPECT_EQ(os::path::getRelativePath(FILE, "/"),
            os::path::getAbsolutePath(FILE).substr(1));
  EXPECT_EQ(os::path::getRelativePath(NESTED_FILE_TXT, COPY_DIRECTORY),
            "../dir" + string::split(NESTED_FILE_TXT, "/dir").back());
  EXPECT_THROW(os::path::getRelativePath(FILE, FILE), IOException);
}

}  // namespace sentinel
