/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/CTestXmlParser.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class CTestXmlParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_CTEST_XML_PARSER_TEST");
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

TEST_F(CTestXmlParserTest, testParsesPassedAndFailedTestcases) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase name="TC1" status="run" />
  <testcase name="TC2" status="fail">
    <failure message="fail" type="" />
  </testcase>
</testsuite>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 1u);
  EXPECT_EQ(passed[0], "TC1");
  ASSERT_EQ(failed.size(), 1u);
  EXPECT_EQ(failed[0], "TC2");
}

TEST_F(CTestXmlParserTest, testIgnoresTestcasesWithOtherStatus) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase name="TC1" status="notrun" />
  <testcase name="TC2" status="disabled" />
</testsuite>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(CTestXmlParserTest, testFailStatusWithoutFailureChildClassifiedAsPassed) {
  // CTestXmlParser checks the <failure> child to classify; this guards against
  // a future refactor that uses the status attribute as the sole signal.
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase name="TC1" status="fail" />
</testsuite>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 1u);
  EXPECT_EQ(passed[0], "TC1");
  EXPECT_TRUE(failed.empty());
}

TEST_F(CTestXmlParserTest, testRejectsMissingRootTestsuite) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<other>
  <testcase name="TC1" status="run" />
</other>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(CTestXmlParserTest, testRejectsMissingTestcase) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
</testsuite>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(CTestXmlParserTest, testRejectsMissingStatusAttribute) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase name="TC1" />
</testsuite>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(CTestXmlParserTest, testRejectsMissingTestcaseName) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase status="run" />
</testsuite>
)";
  CTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(writeXml(xml).string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

}  // namespace sentinel
