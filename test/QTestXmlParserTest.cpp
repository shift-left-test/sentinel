/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/QTestXmlParser.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class QTestXmlParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_QTEST_XML_PARSER_TEST");
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

TEST_F(QTestXmlParserTest, testParsesPassAndFailResults) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="C1">
  <testcase name="TC1" result="pass" />
  <testcase name="TC2" result="fail" />
</testsuite>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 1u);
  EXPECT_EQ(passed[0], "C1.TC1");
  ASSERT_EQ(failed.size(), 1u);
  EXPECT_EQ(failed[0], "C1.TC2");
}

TEST_F(QTestXmlParserTest, testIgnoresUnknownResultValue) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="C1">
  <testcase name="TC1" result="skip" />
  <testcase name="TC2" result="xfail" />
</testsuite>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(QTestXmlParserTest, testRejectsMissingRootTestsuite) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<other name="C1">
  <testcase name="TC1" result="pass" />
</other>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(QTestXmlParserTest, testRejectsMissingTestcase) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="C1">
</testsuite>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(QTestXmlParserTest, testRejectsMissingResultAttribute) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="C1">
  <testcase name="TC1" />
</testsuite>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(QTestXmlParserTest, testRejectsMissingTestsuiteName) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase name="TC1" result="pass" />
</testsuite>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(QTestXmlParserTest, testRejectsMissingTestcaseName) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="C1">
  <testcase result="pass" />
</testsuite>
)";
  QTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

}  // namespace sentinel
