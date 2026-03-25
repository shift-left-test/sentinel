/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "sentinel/SourceLine.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class SourceLineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_SOURCELINETEST_TMP_DIR");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mFileA = mBase / "fileA.cpp";
    mFileB = mBase / "fileB.cpp";
    std::ofstream(mFileA.string()).close();
    std::ofstream(mFileB.string()).close();
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  fs::path mBase;
  fs::path mFileA;
  fs::path mFileB;
};

TEST_F(SourceLineTest, testGetPath) {
  SourceLine sl(mFileA, 10);
  EXPECT_EQ(mFileA, sl.getPath());
}

TEST_F(SourceLineTest, testGetLineNumber) {
  SourceLine sl(mFileA, 42);
  EXPECT_EQ(42u, sl.getLineNumber());
}

TEST_F(SourceLineTest, testEqualityOperatorReturnsTrueForSamePathAndLine) {
  SourceLine sl1(mFileA, 5);
  SourceLine sl2(mFileA, 5);
  EXPECT_TRUE(sl1 == sl2);
}

TEST_F(SourceLineTest, testEqualityOperatorReturnsFalseForDifferentLine) {
  SourceLine sl1(mFileA, 5);
  SourceLine sl2(mFileA, 6);
  EXPECT_FALSE(sl1 == sl2);
}

TEST_F(SourceLineTest, testEqualityOperatorReturnsFalseForDifferentPath) {
  SourceLine sl1(mFileA, 5);
  SourceLine sl2(mFileB, 5);
  EXPECT_FALSE(sl1 == sl2);
}

TEST_F(SourceLineTest, testLessThanOperatorByPath) {
  // mFileA ends with "fileA.cpp", mFileB ends with "fileB.cpp" → A < B lexicographically
  SourceLine slA(mFileA, 1);
  SourceLine slB(mFileB, 1);
  EXPECT_TRUE(slA < slB);
  EXPECT_FALSE(slB < slA);
}

TEST_F(SourceLineTest, testLessThanOperatorByLineWhenSamePath) {
  SourceLine sl1(mFileA, 3);
  SourceLine sl2(mFileA, 7);
  EXPECT_TRUE(sl1 < sl2);
  EXPECT_FALSE(sl2 < sl1);
}

TEST_F(SourceLineTest, testLessThanReturnsFalseForEqualSourceLines) {
  SourceLine sl1(mFileA, 5);
  SourceLine sl2(mFileA, 5);
  EXPECT_FALSE(sl1 < sl2);
}

}  // namespace sentinel
