/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/XMLReport.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

class XMLReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = fs::temp_directory_path() / "SENTINEL_XMLREPORTTEST_TMP_DIR";
    fs::remove_all(BASE);

    OUT_DIR = BASE / "OUT_DIR";
    fs::create_directories(OUT_DIR);

    MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR";
    fs::create_directories(MUT_RESULT_DIR);

    SOURCE_DIR = BASE / "SOURCE_DIR";
    auto NESTED_SOURCE_DIR = SOURCE_DIR / "NESTED_DIR";
    fs::create_directories(NESTED_SOURCE_DIR);

    TARGET_FULL_PATH = SOURCE_DIR / "file1.cpp";
    std::string TARGET_NAME = TARGET_FULL_PATH.filename();
    std::ofstream(TARGET_FULL_PATH).close();

    TARGET_FULL_PATH2 = NESTED_SOURCE_DIR / "file2.cpp";
    std::string TARGET_NAME2 = TARGET_FULL_PATH2.filename();
    std::ofstream(TARGET_FULL_PATH2).close();

    EXPECT_MUT_XML_CONTENT = fmt::format(EXPECT_MUT_XML_CONTENT,
                                         TARGET_NAME, TARGET_FULL_PATH.filename().string(),
                                         TARGET_NAME2, NESTED_SOURCE_DIR.filename().string() + "/" +
                                         TARGET_FULL_PATH2.filename().string());
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  fs::path BASE;
  fs::path OUT_DIR;
  fs::path MUT_RESULT_DIR;
  fs::path SOURCE_DIR;
  fs::path TARGET_FULL_PATH;
  fs::path TARGET_FULL_PATH2;
  std::string EXPECT_MUT_XML_CONTENT =
      R"a1b2(<?xml version="1.0" encoding="UTF-8"?>
<mutations>
    <mutation detected="false">
        <sourceFile>{0}</sourceFile>
        <sourceFilePath>{1}</sourceFilePath>
        <mutatedClass></mutatedClass>
        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>
        <lineNumber>4</lineNumber>
        <mutator>AOR</mutator>
        <killingTest></killingTest>
    </mutation>
    <mutation detected="true">
        <sourceFile>{2}</sourceFile>
        <sourceFilePath>{3}</sourceFilePath>
        <mutatedClass></mutatedClass>
        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>
        <lineNumber>1</lineNumber>
        <mutator>BOR</mutator>
        <killingTest>testAddBit</killingTest>
    </mutation>
</mutations>
)a1b2";
  std::string EXPECT_SKIP_MUT_XML_CONTENT =
      R"a1b2(<?xml version="1.0" encoding="UTF-8"?>
<mutations>
    <mutation detected="false">
        <sourceFile>file1.cpp</sourceFile>
        <sourceFilePath>file1.cpp</sourceFilePath>
        <mutatedClass></mutatedClass>
        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>
        <lineNumber>4</lineNumber>
        <mutator>AOR</mutator>
        <killingTest></killingTest>
    </mutation>
    <mutation detected="true">
        <sourceFile>file2.cpp</sourceFile>
        <sourceFilePath>NESTED_DIR/file2.cpp</sourceFilePath>
        <mutatedClass></mutatedClass>
        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>
        <lineNumber>1</lineNumber>
        <mutator>BOR</mutator>
        <killingTest>testAddBit</killingTest>
    </mutation>
    <mutation detected="skip">
        <sourceFile>file2.cpp</sourceFile>
        <sourceFilePath>NESTED_DIR/file2.cpp</sourceFilePath>
        <mutatedClass></mutatedClass>
        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>
        <lineNumber>1</lineNumber>
        <mutator>BOR</mutator>
        <killingTest></killingTest>
    </mutation>
    <mutation detected="skip">
        <sourceFile>file2.cpp</sourceFile>
        <sourceFilePath>NESTED_DIR/file2.cpp</sourceFilePath>
        <mutatedClass></mutatedClass>
        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>
        <lineNumber>1</lineNumber>
        <mutator>BOR</mutator>
        <killingTest></killingTest>
    </mutation>
</mutations>
)a1b2";
  std::string EXPECT_EMPTY_MUT_XML_CONTENT =
      R"c3d4(<?xml version="1.0" encoding="UTF-8"?>
<mutations/>
)c3d4";
};

TEST_F(XMLReportTest, testMakeXMLReport) {
  MutationResults MRs;
  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MRs.emplace_back(M2, "testAddBit", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MRs.emplace_back(M3, "testAddBit", "", MutationState::RUNTIME_ERROR);

  Mutant M4("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MRs.emplace_back(M4, "testAddBit", "", MutationState::BUILD_FAILURE);

  auto MRPath = MUT_RESULT_DIR / "MutationResult1";
  MRs.save(MRPath);

  XMLReport xmlreport(MRPath, SOURCE_DIR);

  xmlreport.save(OUT_DIR);

  auto outXMLPath = OUT_DIR / "mutations.xml";
  EXPECT_TRUE(fs::exists(outXMLPath));

  std::ifstream t(outXMLPath);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string mutationXMLContent = buffer.str();
  t.close();
  EXPECT_EQ(mutationXMLContent, EXPECT_SKIP_MUT_XML_CONTENT);
}

TEST_F(XMLReportTest, testSaveFailWhenInvalidDirGiven) {
  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  XMLReport xmlreport(MRs, SOURCE_DIR);

  EXPECT_THROW(xmlreport.save(TARGET_FULL_PATH), InvalidArgumentException);
  EXPECT_NO_THROW(xmlreport.save("unknown"));
  ASSERT_TRUE(fs::exists("unknown"));
  fs::remove_all("unknown");
}

TEST_F(XMLReportTest, testMakeXMLReportWhenEmptyMutationResult) {
  MutationResults MRs;
  XMLReport xmlreport(MRs, SOURCE_DIR);
  auto OUT_DIR = BASE / "OUT_DIR_EMPTYMUTATIONRESULT";
  xmlreport.save(OUT_DIR);

  auto outXMLPath = OUT_DIR / "mutations.xml";
  EXPECT_TRUE(fs::exists(outXMLPath));

  std::ifstream t(outXMLPath);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string mutationXMLContent = buffer.str();
  t.close();
  EXPECT_EQ(EXPECT_EMPTY_MUT_XML_CONTENT, mutationXMLContent);
}

}  // namespace sentinel
