/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <experimental/filesystem>
#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/CoverageInfo.hpp"

namespace sentinel {

class CoverageInfoTest : public SampleFileGeneratorForTest {
};

TEST_F(CoverageInfoTest, testCoverWorks) {
  std::string filename = SAMPLECOVERAGE_PATH.string();
  std::string targetfile = SAMPLE1_PATH.string();
  CoverageInfo c{std::vector<std::string>(1, filename)};
  EXPECT_FALSE(c.cover("unknown_file", 123));   // nonexist file
  EXPECT_FALSE(c.cover(targetfile, 39));        // uncovered line
  EXPECT_FALSE(c.cover(targetfile, 40));        // uncovered line
  EXPECT_TRUE(c.cover(targetfile, 33));         // covered line
  EXPECT_TRUE(c.cover(targetfile, 35));         // covered line
  EXPECT_FALSE(c.cover(targetfile, 100));        // line not included in file
}

TEST_F(CoverageInfoTest, testFailWhenUnknownFileGiven) {
  EXPECT_THROW(CoverageInfo c{std::vector<std::string>(1, "unknown.info")}, InvalidArgumentException);
}

}  // namespace sentinel
