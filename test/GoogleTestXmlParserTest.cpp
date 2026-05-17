/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/GoogleTestXmlParser.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class GoogleTestXmlParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_GTEST_XML_PARSER_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  fs::path writeXml(const std::string& content) {
    fs::path path = mBase / "result.xml";
    testutil::writeFile(path, content);
    return path;
  }

  fs::path mBase;
};

TEST_F(GoogleTestXmlParserTest, testParsesPassedAndFailedTestcases) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" status="run" classname="C1" />
    <testcase name="TC2" status="run" classname="C1">
      <failure message="fail" type="" />
    </testcase>
  </testsuite>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 1u);
  EXPECT_EQ(passed[0], "C1.TC1");
  ASSERT_EQ(failed.size(), 1u);
  EXPECT_EQ(failed[0], "C1.TC2");
}

TEST_F(GoogleTestXmlParserTest, testIgnoresTestcasesWithStatusNotRun) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" status="notrun" classname="C1" />
  </testsuite>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(GoogleTestXmlParserTest, testHandlesMultipleTestsuites) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" status="run" classname="C1" />
  </testsuite>
  <testsuite name="C2">
    <testcase name="TC2" status="run" classname="C2" />
  </testsuite>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 2u);
  EXPECT_EQ(passed[0], "C1.TC1");
  EXPECT_EQ(passed[1], "C2.TC2");
}

TEST_F(GoogleTestXmlParserTest, testRejectsMissingRootTestsuites) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<other>
  <testsuite name="C1">
    <testcase name="TC1" status="run" classname="C1" />
  </testsuite>
</other>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(GoogleTestXmlParserTest, testRejectsMissingTestsuite) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(GoogleTestXmlParserTest, testRejectsMissingTestcase) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
  </testsuite>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(GoogleTestXmlParserTest, testRejectsMissingStatusAttribute) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" classname="C1" />
  </testsuite>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(GoogleTestXmlParserTest, testRejectsMissingClassnameAttribute) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" status="run" />
  </testsuite>
</testsuites>
)";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

}  // namespace sentinel
