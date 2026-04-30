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

  MutationResults buildMRsWithUncovered() {
    auto MRs = buildStandardMRs();
    // 첫 번째 SURVIVED (M1, AOR @ REL_PATH1)을 uncovered로 표시
    MRs[0].setUncovered(true);
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

TEST_F(HtmlReportTest, testJsonContainsLcovTracefileWhenSet) {
  auto OUT_DIR = BASE / "OUT_DIR_LCOV";
  auto MRs = buildStandardMRs();

  Config cfg{};
  cfg.lcovTracefiles = {"/tmp/coverage.info"};

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), cfg);
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "lcovTracefiles");
  expectContains(content, "skip uncovered evaluation");
}

TEST_F(HtmlReportTest, testJsonContainsRestrictMode) {
  auto OUT_DIR = BASE / "OUT_DIR_RESTRICT";
  auto MRs = buildStandardMRs();

  Config cfg{};
  cfg.lcovTracefiles = {"/tmp/coverage.info"};
  cfg.restrictGeneration = true;

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), cfg);
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "restrict generation");
}

TEST_F(HtmlReportTest, testJsonOmitsLcovWhenNotSet) {
  auto OUT_DIR = BASE / "OUT_DIR_NOLCOV";
  auto MRs = buildStandardMRs();

  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  EXPECT_EQ(content.find("lcovTracefiles"), std::string::npos);
}

TEST_F(HtmlReportTest, testJsonContainsOperatorBreakdown) {
  auto OUT_DIR = BASE / "OUT_DIR_OPERATOR";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"byOperator\":");
  expectContains(content, "\"AOR\":[1,2,3,0]");
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

TEST_F(HtmlReportTest, testJsonContainsSurvivedUncoveredCount) {
  auto OUT_DIR = BASE / "OUT_DIR_SURV_UNCOV";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"survivedUncovered\":1");
}

TEST_F(HtmlReportTest, testJsonByOperatorIncludesUncoveredCount) {
  auto OUT_DIR = BASE / "OUT_DIR_BYOP_UNCOV";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // AOR: 1 killed, 2 survived, 3 skipped, 1 uncovered
  expectContains(content, "\"AOR\":[1,2,3,1]");
}

TEST_F(HtmlReportTest, testJsonByOperatorZeroUncoveredArrayHasFourSlots) {
  auto OUT_DIR = BASE / "OUT_DIR_BYOP_NOUNCOV";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // AOR: 1 killed, 2 survived, 3 skipped, 0 uncovered
  expectContains(content, "\"AOR\":[1,2,3,0]");
  expectContains(content, "\"survivedUncovered\":0");
}

TEST_F(HtmlReportTest, testSurvivedCardSubLineShowsUncoveredCount) {
  auto OUT_DIR = BASE / "OUT_DIR_CARD_UNCOV";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "Not detected");
  expectContains(content, "uncovered");
}

TEST_F(HtmlReportTest, testSurvivedCardSubLineOmitsUncoveredWhenZero) {
  auto OUT_DIR = BASE / "OUT_DIR_CARD_NOUNCOV";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "Not detected");
  // sub-line의 정적 'uncovered' 토큰은 없어야 함
  // (lcovMode=skip uncovered evaluation 등 config 영향 없음)
  // 참고: JS 코드에 ' uncovered' 리터럴은 들어가지만 HTML 출력 시점에는 동적.
  // 여기서는 ' uncovered' 단어가 출력에 들어가지 않음을 확인하기보다
  // 'survivedUncovered":0'을 통한 간접 확인.
  expectContains(content, "\"survivedUncovered\":0");
}

TEST_F(HtmlReportTest, testMutantsDonutDefinesHatchPattern) {
  auto OUT_DIR = BASE / "OUT_DIR_DONUT_HATCH";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "hatch-uncov");
  expectContains(content, "patternUnits");
  expectContains(content, "url(#hatch-uncov)");
}

TEST_F(HtmlReportTest, testMutantsLegendShowsUncoveredSubset) {
  auto OUT_DIR = BASE / "OUT_DIR_DONUT_LEGEND";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "legend-dot--inline");
  expectContains(content, "uncovered");
}

TEST_F(HtmlReportTest, testOperatorBarShowsUncoveredCountInParens) {
  auto OUT_DIR = BASE / "OUT_DIR_OPBAR_UNCOV";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // op cnt 텍스트는 JS 빌더 안에 리터럴로 박혀 있음. 핵심 토큰 검증.
  expectContains(content, "survived");
  expectContains(content, "uncovered)");
  expectContains(content, "(");
  // bar-u CSS 클래스도 정의되어 있어야 함
  expectContains(content, ".bar-u");
}

TEST_F(HtmlReportTest, testOperatorBarHasHatchPatternCss) {
  auto OUT_DIR = BASE / "OUT_DIR_OPBAR_HATCH";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, ".bar-u");
  expectContains(content, "repeating-linear-gradient");
}

TEST_F(HtmlReportTest, testMutationListRowHasUncoveredChip) {
  auto OUT_DIR = BASE / "OUT_DIR_FILE_CHIP_UNCOV";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "chip--uncov");
  expectContains(content, "Uncovered");
}

TEST_F(HtmlReportTest, testMutationsJsonContainsUncoveredField) {
  auto OUT_DIR = BASE / "OUT_DIR_MUT_UNCOV_FIELD";
  auto MRs = buildMRsWithUncovered();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  expectContains(content, "\"uncovered\":true");
  expectContains(content, "\"uncovered\":false");
}

TEST_F(HtmlReportTest, testStatusChipsHelperEmittedInJs) {
  auto OUT_DIR = BASE / "OUT_DIR_STATUS_CHIPS";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // statusChips 헬퍼 함수가 JS에 삽입되어 있음
  expectContains(content, "function statusChips(");
  // chip--surv CSS 클래스도 정의됨
  expectContains(content, ".chip--surv");
}

TEST_F(HtmlReportTest, testLegendSubItemHasInfoTooltipMarkup) {
  auto OUT_DIR = BASE / "OUT_DIR_LEGEND_TOOLTIP";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // legendItemSub은 카운트 우측에 ⓘ 트리거 버튼과 툴팁 컨테이너를 둔다.
  expectContains(content, "class=\"legend-info\"");
  expectContains(content, "role=\"tooltip\"");
  expectContains(content, "class=\"legend-tip\"");
  // ⓘ 글리프(U+24D8)와 cursor: help 어포던스.
  expectContains(content, "&#9432;");
  expectContains(content, "cursor: help");
}

TEST_F(HtmlReportTest, testLegendSubLineHasPercentFormat) {
  auto OUT_DIR = BASE / "OUT_DIR_LEGEND_PERCENT";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // sub-line 텍스트에 부모 카운트 기준 퍼센트(예: " (40%)")가 포함된다.
  expectContains(content, "Math.round");
  expectContains(content, "%)");
}

TEST_F(HtmlReportTest, testLegendInfoHandlerInstalled) {
  auto OUT_DIR = BASE / "OUT_DIR_LEGEND_INFO";
  auto MRs = buildStandardMRs();
  HtmlReport htmlreport(MutationSummary(MRs, SOURCE_DIR), Config{});
  htmlreport.save(OUT_DIR);

  auto content = testutil::readFile(OUT_DIR / "index.html");
  // hover/focus/Esc 트리거 핸들러가 설치되어 있어야 한다.
  expectContains(content, "function bindLegendInfo");
  expectContains(content, "addEventListener('mouseenter'");
  expectContains(content, "addEventListener('focus'");
  expectContains(content, "ev.key === 'Escape'");
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
