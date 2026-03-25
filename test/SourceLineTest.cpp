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
    BASE = testTempDir("SENTINEL_SOURCELINETEST_TMP_DIR");
    fs::remove_all(BASE);
    fs::create_directories(BASE);
    FILE_A = BASE / "fileA.cpp";
    FILE_B = BASE / "fileB.cpp";
    std::ofstream(FILE_A.string()).close();
    std::ofstream(FILE_B.string()).close();
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  fs::path BASE;
  fs::path FILE_A;
  fs::path FILE_B;
};

TEST_F(SourceLineTest, testGetPath) {
  SourceLine sl(FILE_A, 10);
  EXPECT_EQ(FILE_A, sl.getPath());
}

TEST_F(SourceLineTest, testGetLineNumber) {
  SourceLine sl(FILE_A, 42);
  EXPECT_EQ(42u, sl.getLineNumber());
}

TEST_F(SourceLineTest, testEqualityOperatorReturnsTrueForSamePathAndLine) {
  SourceLine sl1(FILE_A, 5);
  SourceLine sl2(FILE_A, 5);
  EXPECT_TRUE(sl1 == sl2);
}

TEST_F(SourceLineTest, testEqualityOperatorReturnsFalseForDifferentLine) {
  SourceLine sl1(FILE_A, 5);
  SourceLine sl2(FILE_A, 6);
  EXPECT_FALSE(sl1 == sl2);
}

TEST_F(SourceLineTest, testEqualityOperatorReturnsFalseForDifferentPath) {
  SourceLine sl1(FILE_A, 5);
  SourceLine sl2(FILE_B, 5);
  EXPECT_FALSE(sl1 == sl2);
}

TEST_F(SourceLineTest, testLessThanOperatorByPath) {
  // FILE_A ends with "fileA.cpp", FILE_B ends with "fileB.cpp" → A < B lexicographically
  SourceLine slA(FILE_A, 1);
  SourceLine slB(FILE_B, 1);
  EXPECT_TRUE(slA < slB);
  EXPECT_FALSE(slB < slA);
}

TEST_F(SourceLineTest, testLessThanOperatorByLineWhenSamePath) {
  SourceLine sl1(FILE_A, 3);
  SourceLine sl2(FILE_A, 7);
  EXPECT_TRUE(sl1 < sl2);
  EXPECT_FALSE(sl2 < sl1);
}

TEST_F(SourceLineTest, testLessThanReturnsFalseForEqualSourceLines) {
  SourceLine sl1(FILE_A, 5);
  SourceLine sl2(FILE_A, 5);
  EXPECT_FALSE(sl1 < sl2);
}

}  // namespace sentinel
