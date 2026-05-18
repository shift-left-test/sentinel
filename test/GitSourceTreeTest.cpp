/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include "helper/FileTestHelper.hpp"
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

TEST_F(GitSourceTreeTest, testModifyRejectsSiblingDirectoryEscape) {
  // gitRoot's name shares a string prefix with a sibling directory's name.
  // A raw startsWith check would let the sibling pass the containment gate;
  // the component-wise check must reject it.
  fs::path sibling = mBaseDir.parent_path() / (mBaseDir.filename().string() + "_sibling");
  fs::remove_all(sibling);
  fs::create_directories(sibling);
  fs::path outsideFile = sibling / "outside.cpp";
  fs::copy(SAMPLE1_PATH, outsideFile);

  Mutant m{"LCR", outsideFile, "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"};
  GitSourceTree tree(mBaseDir);
  fs::path BACKUP_PATH = mBaseDir / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);

  EXPECT_THROW(tree.modify(m, BACKUP_PATH), IOException);

  fs::remove_all(sibling);
}

TEST_F(GitSourceTreeTest, testModifyWorksWhenValidMutantGiven) {
  Mutant m{"LCR", fs::path(mTmpFileName), "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"};
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
  Mutant nonexistLinePosition{"LCR", fs::path(mTmpFileName), "sumOfEvenPositiveNumber", 10000, 200, 300, 400, "||"};
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

TEST_F(GitSourceTreeTest, testModifyPreservesNoTrailingNewline) {
  // Original file has no trailing newline. After mutation it must still have
  // no trailing newline so build systems sensitive to the property are not
  // impacted by the mutation.
  fs::path src = mBaseDir / "noeol.cpp";
  const std::string content = "int a = 1;\nint b = 2;";  // no trailing newline
  std::ofstream(src) << content;

  // Mutate "1" at line 1, column 9..10 -> "2"
  Mutant m{"AOR", fs::path("noeol.cpp"), "", 1, 9, 1, 10, "2"};
  GitSourceTree tree(mBaseDir);
  fs::path backupPath = mBaseDir / "BACKUP_DIR";
  fs::create_directories(backupPath);
  tree.modify(m, backupPath);

  std::ifstream f(src);
  std::string mutated((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  ASSERT_FALSE(mutated.empty());
  EXPECT_NE('\n', mutated.back());
  EXPECT_EQ("int a = 2;\nint b = 2;", mutated);
}

TEST_F(GitSourceTreeTest, testModifyPreservesTrailingNewline) {
  // Original file ends with a newline. After mutation it must still end with
  // exactly one newline.
  fs::path src = mBaseDir / "withnl.cpp";
  const std::string content = "int a = 1;\nint b = 2;\n";  // ends with newline
  std::ofstream(src) << content;

  Mutant m{"AOR", fs::path("withnl.cpp"), "", 1, 9, 1, 10, "2"};
  GitSourceTree tree(mBaseDir);
  fs::path backupPath = mBaseDir / "BACKUP_DIR";
  fs::create_directories(backupPath);
  tree.modify(m, backupPath);

  std::ifstream f(src);
  std::string mutated((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  ASSERT_FALSE(mutated.empty());
  EXPECT_EQ('\n', mutated.back());
  EXPECT_EQ("int a = 2;\nint b = 2;\n", mutated);
}

TEST_F(GitSourceTreeTest, testBackupWorks) {
  // create a temporary copy of target file
  auto tempSubDirPath = mBaseDir / "SUB_DIR";
  fs::create_directories(tempSubDirPath);

  auto tempFilename = tempSubDirPath / "TMP_FILE";
  fs::copy(SAMPLE1_PATH, tempFilename, fs::copy_options::overwrite_existing);

  Mutant m{"LCR", fs::path("SUB_DIR") / "TMP_FILE", "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"};
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

TEST_F(GitSourceTreeTest, testModifyLeavesNoTemporaryFile) {
  // The atomic-rename path writes through a sibling temp file;
  // a successful modify must not leak it.
  Mutant m{"LCR", fs::path(mTmpFileName), "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"};
  GitSourceTree tree(mBaseDir);
  fs::path BACKUP_PATH = mBaseDir / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);

  tree.modify(m, BACKUP_PATH);

  fs::path tempPath = mTmpFilePath;
  tempPath += GitSourceTree::kMutatedTempSuffix;
  EXPECT_FALSE(fs::exists(tempPath));
}

TEST_F(GitSourceTreeTest, testModifyKeepsOriginalIntactWhenWriteThrows) {
  // Intentionally abuses std::string::substr semantics: setting last.column
  // far past the actual line length makes line.substr(last.column - 1) throw
  // std::out_of_range from deep inside the write loop, exercising the
  // rollback path. If production code later adds a column-range guard, this
  // test must be updated to trigger the throw a different way.
  const std::string before = testutil::readFile(mTmpFilePath);

  Mutant m{"LCR", fs::path(mTmpFileName), "f", 1, 1, 1, 99999, "||"};
  GitSourceTree tree(mBaseDir);
  fs::path BACKUP_PATH = mBaseDir / "sentineltest_backup";
  fs::create_directories(BACKUP_PATH);

  EXPECT_THROW(tree.modify(m, BACKUP_PATH), std::out_of_range);

  EXPECT_EQ(before, testutil::readFile(mTmpFilePath));

  fs::path tempPath = mTmpFilePath;
  tempPath += GitSourceTree::kMutatedTempSuffix;
  EXPECT_FALSE(fs::exists(tempPath));
}

}  // namespace sentinel
