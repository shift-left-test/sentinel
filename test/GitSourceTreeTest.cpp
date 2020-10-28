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
#include "SampleFileGeneratorForTest.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/os.hpp"


namespace sentinel {

class GitSourceTreeTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    BASE_DIR = os::tempDirectory("fixture");
    TMP_FILE_PATH = os::tempPath(BASE_DIR + "/");
    TMP_FILE_NAME = os::path::filename(TMP_FILE_PATH);
    os::copyFile(SAMPLE1_PATH, TMP_FILE_PATH);
  }

  void TearDown() override {
    os::removeDirectories(BASE_DIR);
    SampleFileGeneratorForTest::TearDown();
  }

  std::string BASE_DIR;
  std::string TMP_FILE_PATH;
  std::string TMP_FILE_NAME;
};

TEST_F(GitSourceTreeTest, testConstructorFailWhenInvalidDirGiven) {
  EXPECT_THROW(GitSourceTree tree("unknown"), IOException);
}

TEST_F(GitSourceTreeTest, testModifyWorksWhenValidMutantGiven) {
  Mutant m{"LCR", TMP_FILE_PATH, "sumOfEvenPositiveNumber",
            58, 29, 58, 31, "||"};
  GitSourceTree tree(BASE_DIR);
  std::string BACKUP_PATH = os::path::join(BASE_DIR, "sentineltest_backup");
  os::createDirectory(BACKUP_PATH);
  tree.modify(m, BACKUP_PATH);

  // backup exists
  EXPECT_TRUE(os::path::exists(os::path::join(BACKUP_PATH, TMP_FILE_NAME)));

  // mutation is applied correctly
  std::ifstream mutatedFile(TMP_FILE_PATH);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  std::size_t lineIdx = 0;
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
}

TEST_F(GitSourceTreeTest, testModifyWorksWhenInvalidMutantGiven) {
  // If position does not exist, no changes should be made.
  Mutant nonexistLinePosition{"LCR", TMP_FILE_PATH, "sumOfEvenPositiveNumber",
                               100, 200, 300, 400, "||"};
  GitSourceTree tree(BASE_DIR);

  std::string BACKUP_PATH = os::path::join(BASE_DIR, "sentineltest_backup");
  os::createDirectory(BACKUP_PATH);
  tree.modify(nonexistLinePosition, BACKUP_PATH);

  EXPECT_TRUE(os::path::exists(os::path::join(BACKUP_PATH, TMP_FILE_NAME)));
  std::ifstream mutatedFile(TMP_FILE_PATH);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  while (std::getline(mutatedFile, mutatedFileLine) &&
         std::getline(origFile, origFileLine)) {
    EXPECT_EQ(mutatedFileLine, origFileLine);
  }
  origFile.close();
  mutatedFile.close();
  os::removeDirectories(BACKUP_PATH);

  Mutant m{"LCR", SAMPLE1_PATH, "", 1, 2, 3, 4, "||"};
  GitSourceTree tree2(BASE_DIR);
  EXPECT_THROW(tree2.modify(m, BACKUP_PATH), IOException);
}

TEST_F(GitSourceTreeTest, testBackupWorks) {
  // create a temporary copy of target file
  std::string tempSubDirPath = os::tempPath(BASE_DIR + "/");
  std::string tempSubDirName = os::path::filename(tempSubDirPath);
  os::createDirectory(tempSubDirPath);

  std::string tempFilename = os::tempFilename(tempSubDirPath + "/");
  std::string filename = os::path::filename(tempFilename);
  os::copyFile(SAMPLE1_PATH, tempFilename);

  Mutant m{"LCR", tempFilename, "sumOfEvenPositiveNumber",
            58, 29, 58, 31, "||"};
  GitSourceTree tree(BASE_DIR);
  std::string backupPath = os::tempDirectory(BASE_DIR + "/");

  tree.modify(m, backupPath);

  // backup exists
  EXPECT_TRUE(os::path::exists(os::path::join(backupPath,
                                              tempSubDirName,
                                              filename)));

  // mutation is applied correctly
  std::ifstream mutatedFile(tempFilename);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  std::size_t lineIdx = 0;
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
}

}  // namespace sentinel
