/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "helper/FileTestHelper.hpp"
#include "helper/SentinelReportTestBase.hpp"
#include "sentinel/HtmlReport.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class HtmlReportTest : public SentinelReportTestBase {
 protected:
  void SetUp() override {
    setUpDirectories("SENTINEL_HTMLREPORTTEST_TMP_DIR");
    writeFile(TARGET_FULL_PATH, TARGET_CONTENT);
    writeFile(TARGET_FULL_PATH2, TARGET_CONTENT2);
    writeFile(TARGET_FULL_PATH3, TARGET_CONTENT3);
    writeFile(TARGET_FULL_PATH4, TARGET_CONTENT4);
  }

  void TearDown() override {
    tearDownBase();
  }

  void writeFile(const std::string& path, const std::string& contents) {
    testutil::writeFile(path, contents);
  }

  std::string TARGET_CONTENT =
      R"(#include <iostream>
int add(int a, int b) {
  return a + b;
})";
  std::string TARGET_CONTENT2 =
      R"(int bitwiseOR(int a, int b) {
  return a | b;
})";
  std::string TARGET_CONTENT3 =
      R"(//a & b
int bitwiseAND(int a, int b){
  return a & b;
}

// a - b
int minus(int a, int b){
  return a - b;
})";
  std::string TARGET_CONTENT4 =
      R"(int multiply(int a, int b) {
  return a * b;
})";
  fs::path REL_PATH1 = fs::path("NESTED_DIR1/NESTED_DIR") / "target1_veryVeryVeryVeryVerylongFilePath.cpp";
  fs::path REL_PATH2 = fs::path("NESTED_DIR2") / "target2.cpp";
  fs::path REL_PATH3 = fs::path("NESTED_DIR2") / "target3.cpp";
  fs::path REL_PATH4 = fs::path("target4.cpp");
  void expectContains(const std::string& content, const std::string& substr) {
    EXPECT_NE(content.find(substr), std::string::npos)
        << "Expected to find: " << substr;
  }

  void verifyRootIndexHtml(const std::string& path) {
    EXPECT_TRUE(fs::exists(path));
    auto content = testutil::readFile(path);
    expectContains(content, "Mutation Testing Report");
    expectContains(content, "Mutation Score");
    expectContains(content, "60%");
    expectContains(content, "3 / 5 valid mutants");
    expectContains(content, "Killed");
    expectContains(content, "Survived");
    expectContains(content, "Skipped");
    expectContains(content, "Mutants");
    expectContains(content, "By Operator");
    expectContains(content, "Duration");
    expectContains(content, "By State");
    expectContains(content, "Run Configuration");
    expectContains(content, "Breakdown by File");
    expectContains(content, "Generated:");
    expectContains(content, "./srcDir/index.html");
    expectContains(content, "./srcDir/NESTED_DIR1.NESTED_DIR/index.html");
    expectContains(content, "NESTED_DIR1/NESTED_DIR");
    expectContains(content, "./srcDir/NESTED_DIR2/index.html");
    expectContains(content, "NESTED_DIR2");
    expectContains(content, "AOR (Arithmetic Operator Replacement)");
    expectContains(content, "BOR (Bitwise Operator Replacement)");
    EXPECT_EQ(content.find("stroke-linecap=\"round\""), std::string::npos);
    expectContains(content, "sentinel");
  }

  void verifyDirIndexHtml(const std::string& path, const std::string& dirName,
                          const std::vector<std::string>& expectedFiles,
                          const std::string& coveragePct,
                          std::size_t numerator, std::size_t denominator) {
    EXPECT_TRUE(fs::exists(path));
    auto content = testutil::readFile(path);
    expectContains(content, "Mutation Testing Report");
    expectContains(content, dirName);
    expectContains(content, "Mutation Score");
    expectContains(content, coveragePct);
    expectContains(content, std::to_string(numerator) + " / " +
                            std::to_string(denominator) + " valid mutants");
    expectContains(content, "Killed");
    expectContains(content, "Survived");
    expectContains(content, "Breakdown by File");
    for (const auto& file : expectedFiles) {
      expectContains(content, file);
    }
    expectContains(content, "sentinel");
  }

  void verifySrcHtml(const std::string& path,
                     const std::string& fileName,
                     const std::vector<std::string>& expectedSourceLines,
                     const std::vector<std::string>& expectedOperators,
                     const std::vector<std::string>& expectedTests) {
    EXPECT_TRUE(fs::exists(path));
    auto content = testutil::readFile(path);
    expectContains(content, "Mutation Testing Report");
    expectContains(content, fileName);
    expectContains(content, "Mutation Score");
    expectContains(content, "Killed");
    expectContains(content, "Survived");
    expectContains(content, "Mutants");
    expectContains(content, "Source Code");
    for (const auto& line : expectedSourceLines) {
      expectContains(content, line);
    }
    if (!expectedOperators.empty()) {
      expectContains(content, "Active Mutators");
      for (const auto& op : expectedOperators) {
        expectContains(content, op);
      }
    }
    if (!expectedTests.empty()) {
      expectContains(content, "Tests Examined");
      for (const auto& test : expectedTests) {
        expectContains(content, test);
      }
    }
    expectContains(content, "sentinel");
  }
};

TEST_F(HtmlReportTest, testMakeHtmlReport) {
  auto OUT_DIR = BASE / "OUT_DIR_MAKEHTMLREPORT1";
  fs::create_directories(OUT_DIR);

  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR";
  fs::create_directories(MUT_RESULT_DIR);

  std::string nestedSourceDir =
      NESTED_SOURCE_DIR.parent_path().filename().string() + "." + NESTED_SOURCE_DIR.filename().string();

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "-");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", REL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "&");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", REL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "|");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::KILLED);

  Mutant M4("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "+");
  MRs.emplace_back(M4, "", "", MutationState::SURVIVED);

  Mutant M5("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::BUILD_FAILURE);

  Mutant M6("AOR", REL_PATH4, "multiply", 2, 12, 2, 13, "/");
  MRs.emplace_back(M6, "testMultiply", "", MutationState::KILLED);

  Mutant M7("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M7, "", "", MutationState::RUNTIME_ERROR);

  Mutant M8("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M8, "", "", MutationState::TIMEOUT);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  HtmlReport htmlreport(MutationSummary(MRPath, SOURCE_DIR), Config{});

  htmlreport.save(OUT_DIR);

  // Root index.html
  verifyRootIndexHtml(OUT_DIR / "index.html");

  // Source root directory index
  verifyDirIndexHtml(OUT_DIR / "srcDir" / "index.html", ".",
                     {"target4.cpp"}, "100%", 1, 1);

  // Nested directory 1 index
  verifyDirIndexHtml(OUT_DIR / "srcDir" / nestedSourceDir / "index.html",
                     "NESTED_DIR1/NESTED_DIR",
                     {"target1_veryVeryVeryVeryVerylongFilePath.cpp"},
                     "0%", 0, 1);

  // Nested directory 2 index
  verifyDirIndexHtml(
      OUT_DIR / "srcDir" / NESTED_SOURCE_DIR2.filename() / "index.html",
      "NESTED_DIR2", {"target2.cpp", "target3.cpp"}, "66%", 2, 3);

  // Source file: target1 (1 survived)
  verifySrcHtml(
      OUT_DIR / "srcDir" / nestedSourceDir
          / (TARGET_FULL_PATH.filename().string() + ".html"),
      "target1_veryVeryVeryVeryVerylongFilePath.cpp",
      {"#include &lt;iostream&gt;", "int add(int a, int b)", "return a + b"},
      {"AOR (Arithmetic Operator Replacement)"}, {});

  // Source file: target2 (1 killed)
  verifySrcHtml(
      OUT_DIR / "srcDir" / NESTED_SOURCE_DIR2.filename()
          / (TARGET_FULL_PATH2.filename().string() + ".html"),
      "target2.cpp",
      {"int bitwiseOR(int a, int b)", "return a | b"},
      {"BOR (Bitwise Operator Replacement)"}, {"testBitwiseOR"});

  // Source file: target3 (1 killed, 1 survived)
  verifySrcHtml(
      OUT_DIR / "srcDir" / NESTED_SOURCE_DIR2.filename()
          / (TARGET_FULL_PATH3.filename().string() + ".html"),
      "target3.cpp",
      {"int bitwiseAND(int a, int b)", "return a &amp; b",
       "int minus(int a, int b)", "return a - b"},
      {"AOR (Arithmetic Operator Replacement)",
       "BOR (Bitwise Operator Replacement)"},
      {"testBitwiseAND", "testBitwiseOP"});

  // Source file: target4 (1 killed)
  verifySrcHtml(
      OUT_DIR / "srcDir"
          / (TARGET_FULL_PATH4.filename().string() + ".html"),
      "target4.cpp",
      {"int multiply(int a, int b)", "return a * b"},
      {"AOR (Arithmetic Operator Replacement)"}, {"testMultiply"});
}

TEST_F(HtmlReportTest, testConstructorFailWhenInvalidPathGiven) {
  EXPECT_THROW(HtmlReport(MutationSummary("unknown", "unknown"), Config{}), InvalidArgumentException);
}

TEST_F(HtmlReportTest, testMakeHtmlReportWhenEmptyMutationResult) {
  MutationResults MRs;
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  auto OUT_DIR = BASE / "OUT_DIR_EMPTYMUTATIONRESULT";
  htmlreport.save(OUT_DIR);
  auto outXMLPath = OUT_DIR / "index.html";
  EXPECT_TRUE(fs::exists(outXMLPath));

  auto content = testutil::readFile(outXMLPath);
  expectContains(content, "Mutation Testing Report");
  expectContains(content, "no mutated file");
  expectContains(content, "sentinel");
}

TEST_F(HtmlReportTest, testSaveFailWhenInvalidDirPathGiven) {
  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});

  EXPECT_THROW(htmlreport.save(TARGET_FULL_PATH), InvalidArgumentException);
  EXPECT_NO_THROW(htmlreport.save("unknown"));
  ASSERT_TRUE(fs::exists("unknown"));
  fs::remove_all("unknown");

  std::string nestedSourceDir =
      NESTED_SOURCE_DIR.parent_path().filename().string() + "." + NESTED_SOURCE_DIR.filename().string();

  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDDIRPATHGIVEN";
  fs::create_directories(OUT_DIR / "srcDir");
  auto ERR_FILE = OUT_DIR / "srcDir" / nestedSourceDir;
  std::ofstream(ERR_FILE).close();
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);
}

TEST_F(HtmlReportTest, testSaveFailWhenInvalidSourcePath) {
  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDSOURCEPATH";
  fs::create_directories(OUT_DIR);
  auto tmpPath = TARGET_FULL_PATH;
  tmpPath.concat("_tmpPath");
  fs::copy(TARGET_FULL_PATH, tmpPath);
  Mutant M1("AOR", tmpPath, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  fs::remove(tmpPath);
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);
}

TEST_F(HtmlReportTest, testSaveFailWhenInvalidLineNumber) {
  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDLINENUMBER";
  fs::create_directories(OUT_DIR);
  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 0, 12, 2, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);

  auto OUT_DIR2 = BASE / "OUT_DIR_SAVEFAILWHENINVALIDLINENUMBER2";
  fs::create_directories(OUT_DIR2);
  Mutant M2("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 1000, 12, 1000, 13, "+");
  MutationResult MR2(M2, "", "", MutationState::SURVIVED);

  MutationResults MRs2;
  MRs2.push_back(MR2);

  HtmlReport htmlreport2(MutationSummary(MRs2, SOURCE_DIR), Config{});
  EXPECT_THROW(htmlreport2.save(OUT_DIR2), InvalidArgumentException);
}

}  // namespace sentinel
