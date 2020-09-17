/*
  MIT License

  Copyright (c) 2020 Loc Duy Phan

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
#include "sentinel/SourceTree.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/os.hpp"

namespace sentinel {

TEST(SourceTreeTest, testModifyWorksWhenValidMutableGiven) {
  std::string targetFilename = "input/sample1/sample1.cpp";

  // create a temporary copy of target file
  std::string tempFilename = os::tempFilename("/tmp/");
  std::string filename = os::path::filename(tempFilename);
  os::copyFile(targetFilename, tempFilename);

  Mutable m{"LCR", tempFilename, "sumOfEvenPositiveNumber",
            58, 29, 58, 31, "||"};
  SourceTree tree;
  os::createDirectory("/tmp/sentineltest_backup");
  tree.modify(m, "/tmp", "/tmp/sentineltest_backup");

  // backup exists
  EXPECT_TRUE(os::path::exists("/tmp/sentineltest_backup/" + filename));

  // mutation is applied correctly
  std::ifstream mutatedFile(tempFilename);
  std::ifstream origFile(targetFilename);
  std::string mutatedFileLine, origFileLine;
  int lineIdx = 0;
  while (std::getline(mutatedFile, mutatedFileLine) &&
         std::getline(origFile, origFileLine)) {
    lineIdx += 1;
    if (lineIdx != 58) {
      EXPECT_EQ(mutatedFileLine, origFileLine);
    } else {
      EXPECT_EQ(mutatedFileLine, "    if ((i & 1) == (1 << 0) || i > 0) {");
    }
  }

  origFile.close();
  mutatedFile.close();
  os::removeDirectories("/tmp/sentineltest_backup");
}

TEST(SourceTreeTest, testModifyWorksWhenInvalidMutableGiven) {
  std::string targetFilename = "input/sample1/sample1.cpp";

  // create a temporary copy of target file
  std::string tempFilename = os::tempFilename("/tmp/");
  std::string filename = os::path::filename(tempFilename);
  os::copyFile(targetFilename, tempFilename);

  // If position does not exist, no changes should be made.
  Mutable nonexistLinePosition{"LCR", tempFilename, "sumOfEvenPositiveNumber",
                               100, 200, 300, 400, "||"};
  SourceTree tree;
  os::createDirectory("/tmp/sentineltest_backup");
  tree.modify(nonexistLinePosition, "/tmp", "/tmp/sentineltest_backup");
  EXPECT_TRUE(os::path::exists("/tmp/sentineltest_backup/" + filename));
  std::ifstream mutatedFile(tempFilename);
  std::ifstream origFile(targetFilename);
  std::string mutatedFileLine, origFileLine;
  while (std::getline(mutatedFile, mutatedFileLine) &&
         std::getline(origFile, origFileLine)) {
    EXPECT_EQ(mutatedFileLine, origFileLine);
  }
  origFile.close();
  mutatedFile.close();
  os::removeDirectories("/tmp/sentineltest_backup");
}

TEST(SourceTreeTest, testBackupWorks) {
  std::string targetFilename = "input/sample1/sample1.cpp";

  // create a temporary copy of target file
  std::string tempSubDirName = os::tempPath();
  std::string tempSubDirPath = "/tmp/" + tempSubDirName + "/";
  os::createDirectory(tempSubDirPath);

  std::string tempFilename = os::tempFilename(tempSubDirPath);
  std::string filename = os::path::filename(tempFilename);
  os::copyFile(targetFilename, tempFilename);

  Mutable m{"LCR", tempFilename, "sumOfEvenPositiveNumber",
            58, 29, 58, 31, "||"};
  SourceTree tree;
  std::string backupPath = os::tempDirectory("/tmp/");

  tree.modify(m, "/tmp", backupPath);

  // backup exists
  EXPECT_TRUE(os::path::exists(os::path::join(backupPath,
                                                              tempSubDirName,
                                                              filename)));

  // mutation is applied correctly
  std::ifstream mutatedFile(tempFilename);
  std::ifstream origFile(targetFilename);
  std::string mutatedFileLine, origFileLine;
  int lineIdx = 0;
  while (std::getline(mutatedFile, mutatedFileLine) &&
         std::getline(origFile, origFileLine)) {
    lineIdx += 1;
    if (lineIdx != 58) {
      EXPECT_EQ(mutatedFileLine, origFileLine);
    } else {
      EXPECT_EQ(mutatedFileLine, "    if ((i & 1) == (1 << 0) || i > 0) {");
    }
  }

  origFile.close();
  mutatedFile.close();
  os::removeDirectories(backupPath);
  os::removeDirectories(tempSubDirPath);
}

}  // namespace sentinel

