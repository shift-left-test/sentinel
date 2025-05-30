/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class GitSourceTreeTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    BASE_DIR =
        fs::temp_directory_path() / "SENTINEL_GITSOURCETREETEST_TMP_DIR";
    fs::remove_all(BASE_DIR);
    fs::create_directories(BASE_DIR);
    TMP_FILE_PATH = BASE_DIR / "sample1.cpp";
    TMP_FILE_NAME = TMP_FILE_PATH.filename();
    fs::copy(SAMPLE1_PATH, TMP_FILE_PATH);
  }

  void TearDown() override {
    fs::remove_all(BASE_DIR);
    SampleFileGeneratorForTest::TearDown();
  }

  fs::path BASE_DIR;
  fs::path TMP_FILE_PATH;
  std::string TMP_FILE_NAME;
};

TEST_F(GitSourceTreeTest, testConstructorFailWhenInvalidDirGiven) {
  EXPECT_THROW(GitSourceTree tree("unknown"), IOException);
}

TEST_F(GitSourceTreeTest, testModifyWorksWhenValidMutantGiven) {
  Mutant m{"LCR", TMP_FILE_PATH, "sumOfEvenPositiveNumber",
            58, 29, 58, 31, "||"};
  GitSourceTree tree(BASE_DIR);
  fs::path BACKUP_PATH = BASE_DIR / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);
  tree.modify(m, BACKUP_PATH);

  // backup exists
  EXPECT_TRUE(fs::exists(BACKUP_PATH / TMP_FILE_NAME));

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
                               10000, 200, 300, 400, "||"};
  GitSourceTree tree(BASE_DIR);

  fs::path BACKUP_PATH = BASE_DIR / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);
  tree.modify(nonexistLinePosition, BACKUP_PATH);

  EXPECT_TRUE(fs::exists(BACKUP_PATH / TMP_FILE_NAME));
  std::ifstream mutatedFile(TMP_FILE_PATH);
  std::ifstream origFile(SAMPLE1_PATH);
  std::string mutatedFileLine, origFileLine;
  while (std::getline(mutatedFile, mutatedFileLine) &&
         std::getline(origFile, origFileLine)) {
    EXPECT_EQ(mutatedFileLine, origFileLine);
  }
  origFile.close();
  mutatedFile.close();
  fs::remove_all(BACKUP_PATH);

  Mutant m{"LCR", SAMPLE1_PATH, "", 1, 2, 3, 4, "||"};
  GitSourceTree tree2(BASE_DIR);
  EXPECT_THROW(tree2.modify(m, BACKUP_PATH), IOException);
}

TEST_F(GitSourceTreeTest, testBackupWorks) {
  // create a temporary copy of target file
  auto tempSubDirPath = BASE_DIR / "SUB_DIR";
  std::string tempSubDirName = tempSubDirPath.filename();
  fs::create_directories(tempSubDirPath);

  auto tempFilename = tempSubDirPath / "TMP_FILE";
  std::string filename = tempFilename.filename();
  fs::copy(SAMPLE1_PATH, tempFilename, fs::copy_options::overwrite_existing);

  Mutant m{"LCR", tempFilename, "sumOfEvenPositiveNumber",
            58, 29, 58, 31, "||"};
  GitSourceTree tree(BASE_DIR);
  auto backupPath = BASE_DIR / "BACKUP_DIR";
  fs::create_directories(backupPath);

  tree.modify(m, backupPath);

  // backup exists
  EXPECT_TRUE(fs::exists(backupPath / tempSubDirName / filename));

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
