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
#include <experimental/filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class OsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    using os::tempFilename;
    using os::tempFilename;
    using os::tempPath;
    using os::tempDirectory;

    BASE = tempDirectory("fixture");
    FILE = tempFilename(BASE / "file");
    FILE_XML = tempFilename(BASE / "file", ".xml");
    DIRECTORY = tempDirectory(BASE / "dir");
    COPY_DIRECTORY = tempDirectory(BASE / "dir");
    NESTED_FILE = tempFilename(DIRECTORY / "file");
    NESTED_FILE_TXT = tempFilename(DIRECTORY / "file", ".TXT");
    NESTED_DIRECTORY = tempDirectory(DIRECTORY / "dir");
    UNKNOWN_PATH = "unknown/unknown";
    UNKNOWN_NESTED_PATH = tempPath(DIRECTORY / "unknown");
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  fs::path BASE;
  fs::path FILE;
  std::string FILE_XML;
  std::string NESTED_FILE;
  std::string NESTED_FILE_TXT;
  fs::path DIRECTORY;
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

TEST_F(OsTest, testTempPath) {
  std::string tempname1 = os::tempPath("temp");
  std::string tempname2 = os::tempPath("temp");
  EXPECT_NE(tempname1.compare(tempname2), 0);
}

TEST_F(OsTest, testTempDirectory) {
  std::string tempname1 = os::tempDirectory("");
  std::string tempname2 = os::tempDirectory("");

  EXPECT_NE(tempname1.compare(tempname2), 0);
  EXPECT_TRUE(fs::is_directory(tempname1));
  EXPECT_TRUE(fs::is_directory(tempname2));

  fs::remove_all(tempname1);
  fs::remove_all(tempname2);

  EXPECT_THROW(os::tempDirectory(UNKNOWN_PATH), IOException);
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
               fs::filesystem_error);
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

TEST_F(OsTest, testRelativePath) {
  EXPECT_EQ(os::path::getRelativePath(BASE, BASE), ".");
  EXPECT_EQ(os::path::getRelativePath(FILE, BASE), FILE.filename());
  EXPECT_EQ(os::path::getRelativePath(FILE, "/"),
            fs::canonical(FILE).string().substr(1));
  EXPECT_EQ(os::path::getRelativePath(NESTED_FILE_TXT, COPY_DIRECTORY),
            "../dir" + string::split(NESTED_FILE_TXT, "/dir").back());
}

}  // namespace sentinel
