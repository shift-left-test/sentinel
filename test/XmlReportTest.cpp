/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/XmlReport.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class XmlReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_XMLREPORTTEST_TMP_DIR");
    fs::remove_all(mBase);

    mOutDir = mBase / "OUT_DIR";
    fs::create_directories(mOutDir);

    mMutResultDir = mBase / "MUT_RESULT_DIR";
    fs::create_directories(mMutResultDir);

    mSourceDir = mBase / "SOURCE_DIR";
    auto nestedSourceDir = mSourceDir / "NESTED_DIR";
    fs::create_directories(nestedSourceDir);

    mTargetFullPath = mSourceDir / "file1.cpp";
    std::ofstream(mTargetFullPath).close();

    mTargetFullPath2 = nestedSourceDir / "file2.cpp";
    std::ofstream(mTargetFullPath2).close();

    mRelPath1 = fs::path("file1.cpp");
    mRelPath2 = fs::path("NESTED_DIR") / "file2.cpp";

    mExpectMutXmlContent =
        fmt::format(mExpectMutXmlContent, mRelPath1.filename().string(), mRelPath1.string(),
                    mRelPath2.filename().string(), mRelPath2.string());
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  fs::path mBase;
  fs::path mOutDir;
  fs::path mMutResultDir;
  fs::path mSourceDir;
  fs::path mTargetFullPath;
  fs::path mTargetFullPath2;
  fs::path mRelPath1;
  fs::path mRelPath2;
  std::string mExpectMutXmlContent =
      R"xml(<?xml version="1.0" encoding="UTF-8"?>
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
)xml";
  std::string EXPECT_SKIP_MUT_XML_CONTENT =
      R"xml(<?xml version="1.0" encoding="UTF-8"?>
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
)xml";
  std::string EXPECT_EMPTY_MUT_XML_CONTENT =
      R"xml2(<?xml version="1.0" encoding="UTF-8"?>
<mutations/>
)xml2";
};

TEST_F(XmlReportTest, testMakeXmlReport) {
  MutationResults MRs;
  Mutant M1("AOR", mRelPath1, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", mRelPath2, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MRs.emplace_back(M2, "testAddBit", "", MutationState::KILLED);

  Mutant M3("BOR", mRelPath2, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MRs.emplace_back(M3, "testAddBit", "", MutationState::RUNTIME_ERROR);

  Mutant M4("BOR", mRelPath2, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MRs.emplace_back(M4, "testAddBit", "", MutationState::BUILD_FAILURE);

  auto MRPath = mMutResultDir / "MutationResult1";
  MRs.save(MRPath);

  XmlReport xmlreport(MutationSummary(MRPath, mSourceDir));

  xmlreport.save(mOutDir);

  auto outXMLPath = mOutDir / "mutations.xml";
  EXPECT_TRUE(fs::exists(outXMLPath));

  EXPECT_EQ(testutil::readFile(outXMLPath), EXPECT_SKIP_MUT_XML_CONTENT);
}

TEST_F(XmlReportTest, testSaveFailWhenInvalidDirGiven) {
  Mutant M1("AOR", mRelPath1, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  XmlReport xmlreport(MutationSummary(MRs, mSourceDir));

  EXPECT_THROW(xmlreport.save(mTargetFullPath), InvalidArgumentException);
  EXPECT_NO_THROW(xmlreport.save("unknown"));
  ASSERT_TRUE(fs::exists("unknown"));
  fs::remove_all("unknown");
}

TEST_F(XmlReportTest, testSaveFailWhenReadOnlyDir) {
  MutationResults MRs;
  XmlReport xmlreport(MutationSummary(MRs, mSourceDir));

  auto readonlyDir = mBase / "READONLY_DIR";
  fs::create_directories(readonlyDir);
  fs::permissions(readonlyDir, fs::perms::owner_read | fs::perms::owner_exec,
                  fs::perm_options::replace);

  EXPECT_THROW(xmlreport.save(readonlyDir), IOException);

  fs::permissions(readonlyDir, fs::perms::owner_all,
                  fs::perm_options::replace);
}

TEST_F(XmlReportTest, testMakeXmlReportWhenEmptyMutationResult) {
  MutationResults MRs;
  XmlReport xmlreport(MutationSummary(MRs, mSourceDir));
  auto outDir = mBase / "OUT_DIR_EMPTYMUTATIONRESULT";
  xmlreport.save(outDir);

  auto outXMLPath = outDir / "mutations.xml";
  EXPECT_TRUE(fs::exists(outXMLPath));

  EXPECT_EQ(EXPECT_EMPTY_MUT_XML_CONTENT, testutil::readFile(outXMLPath));
}

TEST_F(XmlReportTest, testMakeXmlReportWithTimeoutMutation) {
  MutationResults MRs;
  Mutant M1("AOR", mRelPath1, "func", 4, 5, 6, 7, "+");
  MRs.emplace_back(M1, "", "", MutationState::TIMEOUT);

  auto MRPath = mMutResultDir / "MutationResultTimeout";
  MRs.save(MRPath);

  XmlReport xmlreport(MutationSummary(MRPath, mSourceDir));
  auto outDir = mBase / "OUT_DIR_TIMEOUT";
  xmlreport.save(outDir);

  auto content = testutil::readFile(outDir / "mutations.xml");
  EXPECT_NE(std::string::npos, content.find("detected=\"skip\""));
  // Killing test should be empty for skip mutations
  EXPECT_NE(std::string::npos, content.find("<killingTest></killingTest>"));
}

TEST_F(XmlReportTest, testMakeXmlReportMixedStates) {
  MutationResults MRs;
  Mutant M1("AOR", mRelPath1, "func", 4, 5, 6, 7, "+");
  MRs.emplace_back(M1, "testKill", "", MutationState::KILLED);

  Mutant M2("BOR", mRelPath1, "func", 5, 1, 5, 2, "|");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);

  Mutant M3("SDL", mRelPath2, "func", 1, 1, 1, 10, "");
  MRs.emplace_back(M3, "", "", MutationState::BUILD_FAILURE);

  Mutant M4("ROR", mRelPath2, "func", 2, 1, 2, 5, "!=");
  MRs.emplace_back(M4, "", "", MutationState::RUNTIME_ERROR);

  Mutant M5("UOI", mRelPath2, "func", 3, 1, 3, 5, "++");
  MRs.emplace_back(M5, "", "", MutationState::TIMEOUT);

  auto MRPath = mMutResultDir / "MutationResultMixed";
  MRs.save(MRPath);

  XmlReport xmlreport(MutationSummary(MRPath, mSourceDir));
  auto outDir = mBase / "OUT_DIR_MIXED";
  xmlreport.save(outDir);

  auto content = testutil::readFile(outDir / "mutations.xml");
  EXPECT_NE(std::string::npos, content.find("detected=\"true\""));
  EXPECT_NE(std::string::npos, content.find("detected=\"false\""));
  // BUILD_FAILURE, RUNTIME_ERROR, TIMEOUT all become "skip"
  auto firstSkip = content.find("detected=\"skip\"");
  ASSERT_NE(std::string::npos, firstSkip);
  auto secondSkip = content.find("detected=\"skip\"", firstSkip + 1);
  ASSERT_NE(std::string::npos, secondSkip);
  auto thirdSkip = content.find("detected=\"skip\"", secondSkip + 1);
  ASSERT_NE(std::string::npos, thirdSkip);
}

}  // namespace sentinel
