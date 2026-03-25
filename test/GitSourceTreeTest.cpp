/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class GitSourceTreeTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    mBaseDir = testTempDir("SENTINEL_GITSOURCETREETEST_TMP_DIR");
    fs::remove_all(mBaseDir);
    fs::create_directories(mBaseDir);
    mTmpFilePath = mBaseDir / "sample1.cpp";
    mTmpFileName = mTmpFilePath.filename();
    fs::copy(SAMPLE1_PATH, mTmpFilePath);
  }

  void TearDown() override {
    fs::remove_all(mBaseDir);
    SampleFileGeneratorForTest::TearDown();
  }

  fs::path mBaseDir;
  fs::path mTmpFilePath;
  std::string mTmpFileName;
};

TEST_F(GitSourceTreeTest, testConstructorFailWhenInvalidDirGiven) {
  EXPECT_THROW(GitSourceTree tree("unknown"), IOException);
}

TEST_F(GitSourceTreeTest, testModifyWorksWhenValidMutantGiven) {
  Mutant m{"LCR", mTmpFilePath, "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"};
  GitSourceTree tree(mBaseDir);
  fs::path BACKUP_PATH = mBaseDir / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);
  tree.modify(m, BACKUP_PATH);

  // backup exists
  EXPECT_TRUE(fs::exists(BACKUP_PATH / mTmpFileName));

  // mutation is applied correctly
  std::ifstream mutatedFile(mTmpFilePath);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  std::size_t lineIdx = 0;
  while (std::getline(mutatedFile, mutatedFileLine) && std::getline(origFile, origFileLine)) {
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
  Mutant nonexistLinePosition{"LCR", mTmpFilePath, "sumOfEvenPositiveNumber", 10000, 200, 300, 400, "||"};
  GitSourceTree tree(mBaseDir);

  fs::path BACKUP_PATH = mBaseDir / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);
  tree.modify(nonexistLinePosition, BACKUP_PATH);

  EXPECT_TRUE(fs::exists(BACKUP_PATH / mTmpFileName));
  std::ifstream mutatedFile(mTmpFilePath);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  while (std::getline(mutatedFile, mutatedFileLine) && std::getline(origFile, origFileLine)) {
    EXPECT_EQ(mutatedFileLine, origFileLine);
  }
  origFile.close();
  mutatedFile.close();
  fs::remove_all(BACKUP_PATH);

  Mutant m{"LCR", SAMPLE1_PATH, "", 1, 2, 3, 4, "||"};
  GitSourceTree tree2(mBaseDir);
  EXPECT_THROW(tree2.modify(m, BACKUP_PATH), IOException);
}

TEST_F(GitSourceTreeTest, testBackupWorks) {
  // create a temporary copy of target file
  auto tempSubDirPath = mBaseDir / "SUB_DIR";
  fs::create_directories(tempSubDirPath);

  auto tempFilename = tempSubDirPath / "TMP_FILE";
  fs::copy(SAMPLE1_PATH, tempFilename, fs::copy_options::overwrite_existing);

  Mutant m{"LCR", tempFilename, "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"};
  GitSourceTree tree(mBaseDir);
  auto backupPath = mBaseDir / "BACKUP_DIR";
  fs::create_directories(backupPath);

  tree.modify(m, backupPath);

  // backup exists
  std::string tempSubDirName = tempSubDirPath.filename();
  std::string filename = tempFilename.filename();
  EXPECT_TRUE(fs::exists(backupPath / tempSubDirName / filename));

  // mutation is applied correctly
  std::ifstream mutatedFile(tempFilename);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  std::size_t lineIdx = 0;
  while (std::getline(mutatedFile, mutatedFileLine) && std::getline(origFile, origFileLine)) {
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
