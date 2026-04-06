/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

class CoverageInfoTest : public SampleFileGeneratorForTest {};

TEST_F(CoverageInfoTest, testCoverWorks) {
  std::string filename = SAMPLECOVERAGE_PATH.string();
  std::string targetfile = SAMPLE1_PATH.string();
  CoverageInfo c{std::vector<std::string>(1, filename)};
  EXPECT_FALSE(c.cover("unknown_file", 123));  // nonexist file
  EXPECT_FALSE(c.cover(targetfile, 39));  // uncovered line
  EXPECT_FALSE(c.cover(targetfile, 40));  // uncovered line
  EXPECT_TRUE(c.cover(targetfile, 33));  // covered line
  EXPECT_TRUE(c.cover(targetfile, 35));  // covered line
  EXPECT_FALSE(c.cover(targetfile, 100));  // line not included in file
}

TEST_F(CoverageInfoTest, testFailWhenUnknownFileGiven) {
  EXPECT_THROW(CoverageInfo c{std::vector<std::string>(1, "unknown.info")}, InvalidArgumentException);
}

TEST_F(CoverageInfoTest, testMultipleCoverageFilesMergeUnion) {
  namespace fs = std::filesystem;
  auto dir = SAMPLE_BASE / "multi_cov";
  fs::create_directories(dir);

  // Create a second source file for the second coverage file
  auto srcFile1 = fs::absolute(SAMPLE1_PATH).string();
  auto srcFile2Path = SAMPLE1_DIR / "other.cpp";
  {
    std::ofstream f(srcFile2Path);
    f << "int other() { return 0; }\n";
  }
  auto srcFile2 = fs::absolute(srcFile2Path).string();

  auto covA = dir / "a.info";
  auto covB = dir / "b.info";

  // a.info covers sample1.cpp line 33
  {
    std::ofstream f(covA);
    f << "SF:" << srcFile1 << "\n"
      << "DA:33,1\n"
      << "end_of_record\n";
  }
  // b.info covers other.cpp line 1
  {
    std::ofstream f(covB);
    f << "SF:" << srcFile2 << "\n"
      << "DA:1,3\n"
      << "end_of_record\n";
  }

  CoverageInfo c({covA.string(), covB.string()});
  EXPECT_TRUE(c.cover(srcFile1, 33));   // from a.info
  EXPECT_TRUE(c.cover(srcFile2, 1));    // from b.info
  EXPECT_FALSE(c.cover(srcFile1, 39));  // not covered in a.info
  EXPECT_FALSE(c.cover(srcFile2, 2));   // not covered in b.info
}

TEST_F(CoverageInfoTest, testDaCountZeroIsNotCovered) {
  namespace fs = std::filesystem;
  auto dir = SAMPLE_BASE / "zero_cov";
  fs::create_directories(dir);

  auto covFile = dir / "zero.info";
  auto srcFile = fs::absolute(SAMPLE1_PATH).string();

  {
    std::ofstream f(covFile);
    f << "SF:" << srcFile << "\n"
      << "DA:33,0\n"
      << "DA:35,1\n"
      << "end_of_record\n";
  }

  CoverageInfo c({covFile.string()});
  EXPECT_FALSE(c.cover(srcFile, 33));  // count=0 → not covered
  EXPECT_TRUE(c.cover(srcFile, 35));   // count=1 → covered
}

TEST_F(CoverageInfoTest, testCoverReturnsFalseForSourceNotInCoverage) {
  std::string filename = SAMPLECOVERAGE_PATH.string();
  CoverageInfo c({filename});
  // Query a file not mentioned in coverage data
  EXPECT_FALSE(c.cover("/some/other/file.cpp", 1));
  EXPECT_FALSE(c.cover("/some/other/file.cpp", 33));
}

TEST_F(CoverageInfoTest, testEmptyCoverageFileProducesNoCoverage) {
  namespace fs = std::filesystem;
  auto dir = SAMPLE_BASE / "empty_cov";
  fs::create_directories(dir);

  auto covFile = dir / "empty.info";
  { std::ofstream f(covFile); }  // empty file

  CoverageInfo c({covFile.string()});
  EXPECT_FALSE(c.cover(SAMPLE1_PATH.string(), 33));
  EXPECT_FALSE(c.cover(SAMPLE1_PATH.string(), 35));
}

TEST_F(CoverageInfoTest, testMalformedDaLineThrows) {
  namespace fs = std::filesystem;
  auto dir = SAMPLE_BASE / "bad_cov";
  fs::create_directories(dir);

  auto covFile = dir / "bad.info";
  auto srcFile = fs::absolute(SAMPLE1_PATH).string();

  {
    std::ofstream f(covFile);
    f << "SF:" << srcFile << "\n"
      << "DA:abc,def\n"
      << "end_of_record\n";
  }

  EXPECT_THROW(CoverageInfo c({covFile.string()}), std::exception);
}

}  // namespace sentinel
