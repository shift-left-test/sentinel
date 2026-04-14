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
#include "sentinel/exceptions/IOException.hpp"

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
  fs::path REL_PATH1 = fs::path("NESTED_DIR1/NESTED_DIR") /
      "target1_veryVeryVeryVeryVerylongFilePath.cpp";
  fs::path REL_PATH2 = fs::path("NESTED_DIR2") / "target2.cpp";
  fs::path REL_PATH3 = fs::path("NESTED_DIR2") / "target3.cpp";
  fs::path REL_PATH4 = fs::path("target4.cpp");

  void expectContains(const std::string& content,
                      const std::string& substr) {
    EXPECT_NE(content.find(substr), std::string::npos)
        << "Expected to find: " << substr;
  }

  void expectNotContains(const std::string& content,
                         const std::string& substr) {
    EXPECT_EQ(content.find(substr), std::string::npos)
        << "Expected NOT to find: " << substr;
  }

  MutationResults buildStandardMRs() {
    MutationResults MRs;
    Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber",
              3, 12, 3, 13, "-");
    MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

    Mutant M2("BOR", REL_PATH2, "sumOfEvenPositiveNumber",
              2, 12, 2, 13, "&");
    MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

    Mutant M3("BOR", REL_PATH3, "sumOfEvenPositiveNumber",
              3, 12, 3, 13, "|");
    MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "",
                     MutationState::KILLED);

    Mutant M4("AOR", REL_PATH3, "sumOfEvenPositiveNumber",
              8, 12, 8, 13, "+");
    MRs.emplace_back(M4, "", "", MutationState::SURVIVED);

    Mutant M5("AOR", REL_PATH3, "sumOfEvenPositiveNumber",
              8, 12, 8, 13, "-");
    MRs.emplace_back(M5, "", "", MutationState::BUILD_FAILURE);

    Mutant M6("AOR", REL_PATH4, "multiply",
              2, 12, 2, 13, "/");
    MRs.emplace_back(M6, "testMultiply", "", MutationState::KILLED);

    Mutant M7("AOR", REL_PATH3, "sumOfEvenPositiveNumber",
              8, 12, 8, 13, "-");
    MRs.emplace_back(M7, "", "", MutationState::RUNTIME_ERROR);

    Mutant M8("AOR", REL_PATH3, "sumOfEvenPositiveNumber",
              8, 12, 8, 13, "-");
    MRs.emplace_back(M8, "", "", MutationState::TIMEOUT);

    return MRs;
  }
};

TEST_F(HtmlReportTest, testSaveCreatesSingleIndexHtml) {
  auto OUT_DIR = BASE / "OUT_DIR_SINGLE_INDEX";
  fs::create_directories(OUT_DIR);

  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR";
  fs::create_directories(MUT_RESULT_DIR);

  auto MRs = buildStandardMRs();
  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  HtmlReport htmlreport(MutationSummary(MRPath, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  EXPECT_TRUE(fs::exists(OUT_DIR / "index.html"));
  EXPECT_FALSE(fs::exists(OUT_DIR / "style.css"));
  EXPECT_FALSE(fs::exists(OUT_DIR / "srcDir"));
}

TEST_F(HtmlReportTest, testSingleFileContainsEmbeddedCss) {
  auto OUT_DIR = BASE / "OUT_DIR_CSS";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "<style>");
  expectContains(content, ".card__lbl");
  expectContains(content, ".src-tbl");
  expectContains(content, ".mpop");
}

TEST_F(HtmlReportTest, testSingleFileContainsJsonData) {
  auto OUT_DIR = BASE / "OUT_DIR_JSON";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "var DATA =");
  expectContains(content, "\"totalMutations\":");
  expectContains(content, "\"detectedMutations\":");
  expectContains(content, "\"files\":");
}

TEST_F(HtmlReportTest, testJsonContainsSourceContent) {
  auto OUT_DIR = BASE / "OUT_DIR_SOURCE";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "int multiply(int a, int b)");
  expectContains(content, "return a * b");
}

TEST_F(HtmlReportTest, testJsonContainsMutationDetails) {
  auto OUT_DIR = BASE / "OUT_DIR_MUTATION_DETAILS";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"op\":\"AOR\"");
  expectContains(content, "\"state\":\"KILLED\"");
  expectContains(content, "\"killingTest\":\"testMultiply\"");
}

TEST_F(HtmlReportTest, testJsonContainsDirectoryInfo) {
  auto OUT_DIR = BASE / "OUT_DIR_DIR_INFO";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"dirs\":");
  expectContains(content, "NESTED_DIR1/NESTED_DIR");
}

TEST_F(HtmlReportTest, testJsonContainsConfigFields) {
  auto OUT_DIR = BASE / "OUT_DIR_CONFIG";
  auto MRs = buildStandardMRs();

  Config cfg{};
  cfg.seed = 42;
  cfg.threshold = 80.0;

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), cfg);
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"seed\":\"42\"");
  expectContains(content, "\"threshold\":\"80%\"");
}

TEST_F(HtmlReportTest, testJsonContainsOperatorBreakdown) {
  auto OUT_DIR = BASE / "OUT_DIR_OPERATOR";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"byOperator\":");
  expectContains(content, "\"AOR\":[1,2,3]");
}

TEST_F(HtmlReportTest, testSpaJavascriptEmbedded) {
  auto OUT_DIR = BASE / "OUT_DIR_SPA";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "function renderRoot()");
  expectContains(content, "function renderDir(");
  expectContains(content, "function renderFile(");
  expectContains(content, "function route()");
  expectContains(content, "hashchange");
}

TEST_F(HtmlReportTest, testEmptyMutationResultShowsNoMutatedFileMessage) {
  MutationResults MRs;
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  auto OUT_DIR = BASE / "OUT_DIR_EMPTY";
  htmlreport.save(OUT_DIR);

  EXPECT_TRUE(fs::exists(OUT_DIR / "index.html"));
  EXPECT_FALSE(fs::exists(OUT_DIR / "style.css"));

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "no mutated file");
  expectContains(content, "sentinel");
}

TEST_F(HtmlReportTest, testConstructorFailWhenInvalidPathGiven) {
  EXPECT_THROW(
      HtmlReport(MutationSummary("unknown", "unknown"), Config{}),
      InvalidArgumentException);
}

TEST_F(HtmlReportTest, testSaveFailWhenInvalidDirPathGiven) {
  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber",
            3, 12, 3, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});

  EXPECT_THROW(htmlreport.save(TARGET_FULL_PATH),
               InvalidArgumentException);
  EXPECT_NO_THROW(htmlreport.save("unknown"));
  ASSERT_TRUE(fs::exists("unknown"));
  fs::remove_all("unknown");
}

TEST_F(HtmlReportTest, testSaveFailWhenReadOnlyDir) {
  MutationResults MRs;
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});

  auto readonlyDir = BASE / "READONLY_DIR";
  fs::create_directories(readonlyDir);
  fs::permissions(readonlyDir, fs::perms::owner_read | fs::perms::owner_exec,
                  fs::perm_options::replace);

  EXPECT_THROW(htmlreport.save(readonlyDir), IOException);

  fs::permissions(readonlyDir, fs::perms::owner_all,
                  fs::perm_options::replace);
}

TEST_F(HtmlReportTest, testSaveFailWhenInvalidSourcePath) {
  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDSOURCEPATH";
  fs::create_directories(OUT_DIR);
  auto tmpPath = TARGET_FULL_PATH;
  tmpPath.concat("_tmpPath");
  fs::copy(TARGET_FULL_PATH, tmpPath);
  Mutant M1("AOR", tmpPath, "sumOfEvenPositiveNumber",
            3, 12, 3, 13, "+");
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
  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber",
            0, 12, 2, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);

  auto OUT_DIR2 = BASE / "OUT_DIR_SAVEFAILWHENINVALIDLINENUMBER2";
  fs::create_directories(OUT_DIR2);
  Mutant M2("AOR", REL_PATH1, "sumOfEvenPositiveNumber",
            1000, 12, 1000, 13, "+");
  MutationResult MR2(M2, "", "", MutationState::SURVIVED);

  MutationResults MRs2;
  MRs2.push_back(MR2);

  HtmlReport htmlreport2(MutationSummary(MRs2, SOURCE_DIR), Config{});
  EXPECT_THROW(htmlreport2.save(OUT_DIR2), InvalidArgumentException);
}

TEST_F(HtmlReportTest, testSkippedStatesInJson) {
  auto OUT_DIR = BASE / "OUT_DIR_SKIPPED";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"buildFailures\":1");
  expectContains(content, "\"timeouts\":1");
  expectContains(content, "\"runtimeErrors\":1");
}

}  // namespace sentinel
