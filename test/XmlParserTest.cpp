/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "sentinel/CTestXmlParser.hpp"
#include "sentinel/GoogleTestXmlParser.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/QTestXmlParser.hpp"
#include "sentinel/XmlParser.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class XmlParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_XML_PARSER_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    // A prior test that lowered the level (e.g. LoggerTest TearDown -> OFF)
    // would suppress the WARN we capture below; pin it back to INFO.
    Logger::setLevel(Logger::Level::INFO);
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  fs::path writeXml(const std::string& content, const std::string& name = "result.xml") {
    fs::path path = mBase / name;
    testutil::writeFile(path, content);
    return path;
  }

  fs::path mBase;
};

TEST_F(XmlParserTest, testProcessOnNonExistentFileWarns) {
  const fs::path missing = mBase / "does_not_exist.xml";
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;

  testing::internal::CaptureStderr();
  parser.process(missing.string(), &passed, &failed);
  const std::string captured = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
  EXPECT_NE(captured.find("Failed to parse XML"), std::string::npos);
}

TEST_F(XmlParserTest, testProcessOnEmptyFileWarns) {
  const fs::path path = writeXml("");
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;

  testing::internal::CaptureStderr();
  parser.process(path.string(), &passed, &failed);
  const std::string captured = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
  EXPECT_NE(captured.find("Failed to parse XML"), std::string::npos);
}

TEST_F(XmlParserTest, testProcessOnMalformedXmlWarns) {
  const fs::path path = writeXml("<not really xml without a closing");
  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;

  testing::internal::CaptureStderr();
  parser.process(path.string(), &passed, &failed);
  const std::string captured = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
  EXPECT_NE(captured.find("Failed to parse XML"), std::string::npos);
}

TEST_F(XmlParserTest, testProcessDelegatesToNextParserOnSchemaMismatch) {
  // CTest-shaped document handed to a GoogleTest -> CTest chain. GoogleTest's
  // parse() should return false (missing <testsuites> root) and process()
  // should forward to the chained CTest parser, which recognizes the schema.
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuite>
  <testcase name="TC1" status="run" />
</testsuite>
)";
  const fs::path path = writeXml(xml);

  auto google = std::make_shared<GoogleTestXmlParser>();
  auto ctest = std::make_shared<CTestXmlParser>();
  google->setNext(ctest);

  std::vector<std::string> passed;
  std::vector<std::string> failed;
  google->process(path.string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 1u);
  EXPECT_EQ(passed[0], "TC1");
  EXPECT_TRUE(failed.empty());
}

TEST_F(XmlParserTest, testProcessStopsAtFirstSuccessfulParser) {
  // GoogleTest-shaped document handed to the same chain. The first parser
  // should succeed and the chained parser must NOT also append its own data.
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" status="run" classname="C1" />
  </testsuite>
</testsuites>
)";
  const fs::path path = writeXml(xml);

  auto google = std::make_shared<GoogleTestXmlParser>();
  auto ctest = std::make_shared<CTestXmlParser>();
  google->setNext(ctest);

  std::vector<std::string> passed;
  std::vector<std::string> failed;
  google->process(path.string(), &passed, &failed);

  ASSERT_EQ(passed.size(), 1u);
  EXPECT_EQ(passed[0], "C1.TC1");
  EXPECT_TRUE(failed.empty());
}

TEST_F(XmlParserTest, testProcessReturnsEmptyWhenNoParserRecognizesSchema) {
  const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<unknown-root>
  <item />
</unknown-root>
)";
  const fs::path path = writeXml(xml);

  auto google = std::make_shared<GoogleTestXmlParser>();
  auto ctest = std::make_shared<CTestXmlParser>();
  auto qtest = std::make_shared<QTestXmlParser>();
  google->setNext(ctest)->setNext(qtest);

  std::vector<std::string> passed;
  std::vector<std::string> failed;
  google->process(path.string(), &passed, &failed);

  EXPECT_TRUE(passed.empty());
  EXPECT_TRUE(failed.empty());
}

TEST_F(XmlParserTest, testSetNextReturnsTheNewlyChainedParser) {
  auto head = std::make_shared<GoogleTestXmlParser>();
  auto tail = std::make_shared<CTestXmlParser>();
  const auto returned = head->setNext(tail);
  EXPECT_EQ(returned.get(), tail.get());
}

TEST_F(XmlParserTest, testProcessResetsPreviousResultsBetweenCalls) {
  // Reuse the same parser across two files; the second result must not carry
  // over passed/failed names from the first.
  const std::string xml1 = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C1">
    <testcase name="TC1" status="run" classname="C1" />
  </testsuite>
</testsuites>
)";
  const std::string xml2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="C2">
    <testcase name="TC2" status="run" classname="C2" />
  </testsuite>
</testsuites>
)";
  const fs::path p1 = writeXml(xml1, "first.xml");
  const fs::path p2 = writeXml(xml2, "second.xml");

  GoogleTestXmlParser parser;
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  parser.process(p1.string(), &passed, &failed);
  parser.process(p2.string(), &passed, &failed);

  // Guards against the parser carrying TC1 over into the second call.
  ASSERT_EQ(passed.size(), 2u);
  EXPECT_EQ(passed[0], "C1.TC1");
  EXPECT_EQ(passed[1], "C2.TC2");
  EXPECT_TRUE(failed.empty());
}

}  // namespace sentinel
