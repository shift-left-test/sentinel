/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/docGenerator/CssGenerator.hpp"
#include "sentinel/docGenerator/IndexHtmlGenerator.hpp"
#include "sentinel/docGenerator/SrcHtmlGenerator.hpp"
#include "sentinel/operators/MutationOperatorExpansion.hpp"

namespace sentinel {

class DocGeneratorTest : public SampleFileGeneratorForTest {};

TEST_F(DocGeneratorTest, testCssGeneratorStrProducesNonEmptyOutput) {
  CssGenerator gen;
  std::string css = gen.str();
  EXPECT_FALSE(css.empty());
  EXPECT_NE(css.find("margin"), std::string::npos);
  EXPECT_NE(css.find("padding"), std::string::npos);
  EXPECT_NE(css.find(".card__lbl {\n  font-size: .72rem; font-weight: 700; text-transform: uppercase;\n  "
                     "letter-spacing: .06em; color: var(--text-sec); margin-bottom: 6px;\n}"),
            std::string::npos);
  EXPECT_NE(css.find(".panel__t {\n  font-size: .74rem; font-weight: 700; text-transform: uppercase;\n  "
                     "letter-spacing: .06em; color: var(--text-sec); margin-bottom: 16px;\n}"),
            std::string::npos);
  EXPECT_NE(css.find(".bar-name { font-size: .84rem; font-weight: 600; color: var(--text-muted); }"),
            std::string::npos);
}

TEST_F(DocGeneratorTest, testIndexHtmlGeneratorStrProducesValidHTML) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, 0, "");
  std::string html = gen.str();
  EXPECT_NE(html.find("<html"), std::string::npos);
  EXPECT_NE(html.find("</html>"), std::string::npos);
}

TEST_F(DocGeneratorTest, testSrcHtmlGeneratorStrProducesValidHTML) {
  SrcHtmlGenerator gen("sample.cpp", true);
  std::string html = gen.str();
  EXPECT_NE(html.find("<html"), std::string::npos);
  EXPECT_NE(html.find("sample.cpp"), std::string::npos);
}

TEST_F(DocGeneratorTest, testMutationOperatorExpansionUsesReadableSpacing) {
  EXPECT_EQ("AOR (Arithmetic Operator Replacement)",
            mutationOperatorToExpansion("AOR"));
  EXPECT_EQ("UOI (Unary Operator Insertion)",
            mutationOperatorToExpansion("UOI"));
}

// ---------------------------------------------------------------------------
// formatSkippedDetail tests
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testFormatSkippedDetailAllTypes) {
  EXPECT_EQ("", IndexHtmlGenerator::formatSkippedDetail(0, 0, 0));
  EXPECT_EQ("3 timeout", IndexHtmlGenerator::formatSkippedDetail(3, 0, 0));
  EXPECT_EQ("2 build failure", IndexHtmlGenerator::formatSkippedDetail(0, 2, 0));
  EXPECT_EQ("1 runtime error", IndexHtmlGenerator::formatSkippedDetail(0, 0, 1));

  // Two types
  std::string detail = IndexHtmlGenerator::formatSkippedDetail(3, 2, 0);
  EXPECT_NE(detail.find("3 timeout"), std::string::npos);
  EXPECT_NE(detail.find("2 build failure"), std::string::npos);

  detail = IndexHtmlGenerator::formatSkippedDetail(1, 0, 4);
  EXPECT_NE(detail.find("1 timeout"), std::string::npos);
  EXPECT_NE(detail.find("4 runtime error"), std::string::npos);

  detail = IndexHtmlGenerator::formatSkippedDetail(0, 5, 6);
  EXPECT_NE(detail.find("5 build failure"), std::string::npos);
  EXPECT_NE(detail.find("6 runtime error"), std::string::npos);

  // All three types
  detail = IndexHtmlGenerator::formatSkippedDetail(1, 2, 3);
  EXPECT_NE(detail.find("1 timeout"), std::string::npos);
  EXPECT_NE(detail.find("2 build failure"), std::string::npos);
  EXPECT_NE(detail.find("3 runtime error"), std::string::npos);
}

// ---------------------------------------------------------------------------
// pushItemToTable tests
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testPushItemToTableHighCoverage) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, 0, "");
  gen.pushItemToTable("src/foo", 75, 3, 4, 2);
  std::string html = gen.str();
  EXPECT_NE(html.find("Files"), std::string::npos);
  EXPECT_NE(html.find("c-hi"), std::string::npos);
}

TEST_F(DocGeneratorTest, testPushItemToTableMidCoverage) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, 0, "");
  gen.pushItemToTable("src/bar", 50, 2, 4, 1);
  std::string html = gen.str();
  EXPECT_NE(html.find("c-mid"), std::string::npos);
}

TEST_F(DocGeneratorTest, testPushItemToTableLowCoverage) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, 0, "");
  gen.pushItemToTable("src/baz", 20, 1, 5, 3);
  std::string html = gen.str();
  EXPECT_NE(html.find("c-lo"), std::string::npos);
}

TEST_F(DocGeneratorTest, testPushItemToTableNonRootMode) {
  IndexHtmlGenerator gen(false, "src/lib", 1, 30, 1, 3, 0, "");
  gen.pushItemToTable("file.cpp", 30, 1, 3, 0);
  std::string html = gen.str();
  EXPECT_EQ(html.find("text-align:center\">Files"), std::string::npos);
  EXPECT_NE(html.find("c-lo"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Full page tests
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testRootPageWithSummaryShowsPanels) {
  Mutant killedMutant("AOR", SAMPLE1_PATH, "foo::bar", 1, 1, 1, 5, "+");
  Mutant survivedMutant("BOR", SAMPLE1_PATH, "foo::baz", 2, 1, 2, 5, "|");
  Mutant timeoutMutant("LCR", SAMPLE1_PATH, "foo::qux", 3, 1, 3, 5, "&&");

  MutationResults results;
  results.push_back(MutationResult(killedMutant, "test1", "", MutationState::KILLED));
  results.push_back(MutationResult(survivedMutant, "", "", MutationState::SURVIVED));
  results.push_back(MutationResult(timeoutMutant, "", "", MutationState::TIMEOUT));

  MutationSummary summary(results, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;
  cfg.buildCmd = "make";
  cfg.testCmd = "make test";

  IndexHtmlGenerator gen(true, ".", 1, 33, 1, 2, summary, cfg, "2026-01-01", "1.0.0");
  std::string html = gen.str();
  EXPECT_NE(html.find("donut-svg"), std::string::npos);
  EXPECT_NE(html.find("AOR"), std::string::npos);
  EXPECT_NE(html.find("BOR"), std::string::npos);
  EXPECT_NE(html.find("Run Configuration"), std::string::npos);
  EXPECT_NE(html.find("2026-01-01"), std::string::npos);
  EXPECT_NE(html.find("v1.0.0"), std::string::npos);
  EXPECT_NE(html.find("Timeout"), std::string::npos);
}

TEST_F(DocGeneratorTest, testNonRootPageShowsBreadcrumb) {
  IndexHtmlGenerator gen(false, "src/lib", 1, 50, 1, 2, 0, "");
  std::string html = gen.str();
  EXPECT_NE(html.find("src/lib"), std::string::npos);
  EXPECT_NE(html.find("crumb"), std::string::npos);
  EXPECT_NE(html.find("../../style.css"), std::string::npos);
}

TEST_F(DocGeneratorTest, testNonRootEmptyDirNamePage) {
  IndexHtmlGenerator gen(false, "", 1, 50, 1, 2, 0, "");
  std::string html = gen.str();
  EXPECT_NE(html.find("../style.css"), std::string::npos);
  EXPECT_NE(html.find(">.</span>"), std::string::npos);
}

TEST_F(DocGeneratorTest, testRootPageStylePathIsEmpty) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, 0, "");
  std::string html = gen.str();
  EXPECT_NE(html.find("href=\"style.css\""), std::string::npos);
}

TEST_F(DocGeneratorTest, testRootPageWithZeroMutants) {
  MutationResults emptyResults;
  MutationSummary summary(emptyResults, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 0, 0, 0, 0, summary, cfg, "", "");
  std::string html = gen.str();
  EXPECT_NE(html.find("0%"), std::string::npos);
  EXPECT_NE(html.find("0 / 0"), std::string::npos);
}

TEST_F(DocGeneratorTest, testRootPageWithBuildFailureAndRuntimeError) {
  Mutant bfMutant("SDL", SAMPLE1_PATH, "foo", 1, 1, 1, 5, ";");
  Mutant reMutant("UOI", SAMPLE1_PATH, "bar", 2, 1, 2, 5, "++");

  MutationResults results;
  results.push_back(MutationResult(bfMutant, "", "", MutationState::BUILD_FAILURE));
  results.push_back(MutationResult(reMutant, "", "", MutationState::RUNTIME_ERROR));

  MutationSummary summary(results, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 0, 0, 0, summary, cfg, "", "");
  std::string html = gen.str();

  EXPECT_NE(html.find("Build Failure"), std::string::npos);
  EXPECT_NE(html.find("Runtime Error"), std::string::npos);
}

TEST_F(DocGeneratorTest, testConfigSectionShowsOptionalFields) {
  MutationResults emptyResults;
  MutationSummary summary(emptyResults, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;
  cfg.buildCmd = "cmake --build .";
  cfg.testCmd = "ctest";
  cfg.limit = 100;
  cfg.threshold = 80.0;
  cfg.partition = "1/4";
  cfg.seed = 42u;
  cfg.timeout = 30u;

  IndexHtmlGenerator gen(true, ".", 1, 0, 0, 0, summary, cfg, "", "");
  std::string html = gen.str();

  EXPECT_NE(html.find("limit"), std::string::npos);
  EXPECT_NE(html.find("threshold"), std::string::npos);
  EXPECT_NE(html.find("partition"), std::string::npos);
  EXPECT_NE(html.find("seed"), std::string::npos);
  EXPECT_NE(html.find("timeout"), std::string::npos);
  EXPECT_NE(html.find("100"), std::string::npos);
  EXPECT_NE(html.find("80%"), std::string::npos);
  EXPECT_NE(html.find("1/4"), std::string::npos);
  EXPECT_NE(html.find("42"), std::string::npos);
  EXPECT_NE(html.find("30s"), std::string::npos);
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: pushLine CSS class handling
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlPushLineKilledClass) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 1, 0, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "AOR", "a + b", "a - b", true, "TestA");
  gen.pushLine(10, "killed", 1, "int x = a + b;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("class=\"lk\""), std::string::npos)
      << "killed class should produce lk row class";
}

TEST_F(DocGeneratorTest, testSrcHtmlPushLineSurvivedClass) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 1, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "AOR", "a + b", "a - b", false, "");
  gen.pushLine(10, "survived", 1, "int x = a + b;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("class=\"ls\""), std::string::npos)
      << "survived class should produce ls row class";
}

TEST_F(DocGeneratorTest, testSrcHtmlPushLineUncertainClass) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 0, 1, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "AOR", "a + b", "a - b", false, "");
  gen.pushLine(10, "uncertain", 1, "int x = a + b;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("class=\"ls\""), std::string::npos)
      << "uncertain class should produce ls row class";
}

TEST_F(DocGeneratorTest, testSrcHtmlPushLineNoMutations) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 0, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  gen.pushLine(5, "", 0, "int y = 0;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("<td class=\"mi\"></td>"), std::string::npos)
      << "Line with 0 mutations should have empty mi cell";
}

TEST_F(DocGeneratorTest, testSrcHtmlPushLineMutationIndicatorCell) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 1, 0, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "AOR", "a + b", "a - b", true, "TestA");
  gen.pushLine(42, "killed", 1, "int x = a + b;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("data-pop=\"pop-42\""), std::string::npos)
      << "Line with mutations should have data-pop attribute";
  EXPECT_NE(html.find(">1</td>"), std::string::npos)
      << "Mutation indicator should show count";
}

TEST_F(DocGeneratorTest, testSrcHtmlPushLineMultipleMutationsOnSameLine) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 1, 1, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "AOR", "a + b", "a - b", true, "TestA");
  explain.emplace_back(2, "AOR", "a + b", "a * b", false, "");
  gen.pushLine(10, "killed", 2, "int x = a + b;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find(">2</td>"), std::string::npos)
      << "Mutation indicator should show count of 2";
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: popup badge class
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlPopupKilledBadge) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 1, 0, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "AOR (Arithmetic Operator Replacement)", "a + b", "a - b", true, "FooTest.test1");
  gen.pushLine(10, "killed", 1, "int x = a + b;", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("b-k"), std::string::npos) << "Killed mutant should have b-k badge";
  EXPECT_NE(html.find("Killed"), std::string::npos);
  EXPECT_NE(html.find("var(--green)"), std::string::npos)
      << "Killed test should use green color";
}

TEST_F(DocGeneratorTest, testSrcHtmlPopupSurvivedBadge) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 1, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  explain.emplace_back(1, "ROR (Relational Operator Replacement)", "a > b", "a < b", false, "");
  gen.pushLine(10, "survived", 1, "if (a > b) {", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("b-s"), std::string::npos) << "Survived mutant should have b-s badge";
  EXPECT_NE(html.find("Survived"), std::string::npos);
  EXPECT_NE(html.find("var(--text-muted)"), std::string::npos)
      << "Survived test should use muted color";
  EXPECT_NE(html.find("none"), std::string::npos)
      << "Empty killing test should display as 'none'";
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: pushMutation badge
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlPushMutationKilledBadge) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 1, 0, 0, "", "1.0");
  gen.pushMutation(10, true, 1, "FooTest.bar", "AOR");
  std::string html = gen.str();
  EXPECT_NE(html.find("b-k"), std::string::npos);
  EXPECT_NE(html.find("Killed"), std::string::npos);
}

TEST_F(DocGeneratorTest, testSrcHtmlPushMutationSurvivedBadge) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 1, 0, "", "1.0");
  gen.pushMutation(10, false, 1, "", "ROR");
  std::string html = gen.str();
  EXPECT_NE(html.find("b-s"), std::string::npos);
  EXPECT_NE(html.find("Survived"), std::string::npos);
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: panels
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlMutatorsAndTestsPanels) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 1, 0, 0, "", "1.0");
  gen.pushMutator("AOR (Arithmetic Operator Replacement)");
  gen.pushKillingTest("FooTest.testBar");
  std::string html = gen.str();
  EXPECT_NE(html.find("Active Mutators"), std::string::npos);
  EXPECT_NE(html.find("AOR (Arithmetic Operator Replacement)"), std::string::npos);
  EXPECT_NE(html.find("Tests Examined"), std::string::npos);
  EXPECT_NE(html.find("FooTest.testBar"), std::string::npos);
}

TEST_F(DocGeneratorTest, testSrcHtmlNoPanelsWhenEmpty) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 0, 0, "", "1.0");
  std::string html = gen.str();
  // No mutators or tests pushed — panels should not appear
  EXPECT_EQ(html.find("Active Mutators"), std::string::npos);
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: HTML escaping
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlEscapesHtmlCharacters) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 0, 0, 0, "", "1.0");
  std::vector<std::tuple<int, std::string, std::string, std::string, bool, std::string>> explain;
  gen.pushLine(1, "", 0, "if (a < b && c > d) {}", explain);
  std::string html = gen.str();
  EXPECT_NE(html.find("&lt;"), std::string::npos) << "< should be escaped";
  EXPECT_NE(html.find("&gt;"), std::string::npos) << "> should be escaped";
  EXPECT_NE(html.find("&amp;"), std::string::npos) << "& should be escaped";
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: cards section
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlCardsShowCorrectCounts) {
  SrcHtmlGenerator gen("test.cpp", true, "src", 3, 2, 1, "1 timeout", "1.0");
  std::string html = gen.str();
  EXPECT_NE(html.find("Mutation Score"), std::string::npos);
  EXPECT_NE(html.find("Killed"), std::string::npos);
  EXPECT_NE(html.find("Survived"), std::string::npos);
  EXPECT_NE(html.find("Skipped"), std::string::npos);
}

// ---------------------------------------------------------------------------
// SrcHtmlGenerator: breadcrumb
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testSrcHtmlBreadcrumbForSrcRoot) {
  SrcHtmlGenerator gen("test.cpp", true, "src/lib", 0, 0, 0, "", "1.0");
  std::string html = gen.str();
  EXPECT_NE(html.find("crumb"), std::string::npos);
  EXPECT_NE(html.find("test.cpp"), std::string::npos);
}

TEST_F(DocGeneratorTest, testSrcHtmlBreadcrumbForNonSrcRoot) {
  SrcHtmlGenerator gen("test.cpp", false, "src/lib", 0, 0, 0, "", "1.0");
  std::string html = gen.str();
  EXPECT_NE(html.find("crumb"), std::string::npos);
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: formatDuration (private, tested via HTML output)
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testDurationFormattingInHtmlOutput) {
  // Build a summary with known timing to verify formatDuration behavior.
  Mutant m("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  MutationResults results;
  MutationResult r1(m, "test1", "", MutationState::KILLED);
  r1.setBuildSecs(90.0);
  r1.setTestSecs(30.0);
  results.push_back(r1);

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 100, 1, 1, summary, cfg, "", "");
  std::string html = gen.str();

  // 90s = "1m 30s", 30s = "30s"
  EXPECT_NE(html.find("1m 30s"), std::string::npos) << "90s should format as '1m 30s'";
  EXPECT_NE(html.find("30s"), std::string::npos) << "30s should format as '30s'";
}

TEST_F(DocGeneratorTest, testDurationFormattingExactMinutes) {
  Mutant m("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  MutationResults results;
  MutationResult r1(m, "test1", "", MutationState::KILLED);
  r1.setBuildSecs(120.0);
  r1.setTestSecs(60.0);
  results.push_back(r1);

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 100, 1, 1, summary, cfg, "", "");
  std::string html = gen.str();

  // 120s = "2m", 60s = "1m"
  EXPECT_NE(html.find("2m"), std::string::npos) << "120s should format as '2m'";
  EXPECT_NE(html.find("1m"), std::string::npos) << "60s should format as '1m'";
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: donut chart verification
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testDonutChartSegmentsPresent) {
  Mutant m1("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  Mutant m2("BOR", SAMPLE1_PATH, "bar", 2, 1, 2, 5, "|");

  MutationResults results;
  results.push_back(MutationResult(m1, "test1", "", MutationState::KILLED));
  results.push_back(MutationResult(m2, "", "", MutationState::SURVIVED));

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, summary, cfg, "", "");
  std::string html = gen.str();

  // Verify SVG donut segments
  EXPECT_NE(html.find("stroke=\"#0f8a5f\""), std::string::npos) << "Killed segment (green)";
  EXPECT_NE(html.find("stroke=\"#d44030\""), std::string::npos) << "Survived segment (red)";
  EXPECT_NE(html.find("stroke-dasharray"), std::string::npos) << "SVG arc attributes";
}

TEST_F(DocGeneratorTest, testDonutChartWithAllStates) {
  Mutant m1("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  Mutant m2("BOR", SAMPLE1_PATH, "bar", 2, 1, 2, 5, "|");
  Mutant m3("LCR", SAMPLE1_PATH, "baz", 3, 1, 3, 5, "&&");
  Mutant m4("SDL", SAMPLE1_PATH, "qux", 4, 1, 4, 5, "{}");

  MutationResults results;
  results.push_back(MutationResult(m1, "test1", "", MutationState::KILLED));
  results.push_back(MutationResult(m2, "", "", MutationState::SURVIVED));
  results.push_back(MutationResult(m3, "", "", MutationState::TIMEOUT));
  results.push_back(MutationResult(m4, "", "", MutationState::BUILD_FAILURE));

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, summary, cfg, "", "");
  std::string html = gen.str();

  // All state legend entries
  EXPECT_NE(html.find("Killed"), std::string::npos);
  EXPECT_NE(html.find("Survived"), std::string::npos);
  EXPECT_NE(html.find("Timeout"), std::string::npos);
  EXPECT_NE(html.find("Build Failure"), std::string::npos);
  // Skipped segment (gray)
  EXPECT_NE(html.find("stroke=\"#94a0b0\""), std::string::npos);
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: duration donut
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testDurationDonutPresent) {
  Mutant m("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  MutationResults results;
  MutationResult r1(m, "test1", "", MutationState::KILLED);
  r1.setBuildSecs(60.0);
  r1.setTestSecs(30.0);
  results.push_back(r1);

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 100, 1, 1, summary, cfg, "", "");
  std::string html = gen.str();

  // Duration donut should have build (blue) and test (light blue) segments
  EXPECT_NE(html.find("stroke=\"#036098\""), std::string::npos) << "Build segment";
  EXPECT_NE(html.find("stroke=\"#48b0e0\""), std::string::npos) << "Test segment";
  EXPECT_NE(html.find("Build"), std::string::npos);
  EXPECT_NE(html.find("Test"), std::string::npos);
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: config section fields
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testConfigSectionShowsFromAndUncommitted) {
  MutationResults emptyResults;
  MutationSummary summary(emptyResults, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;
  cfg.buildCmd = "make";
  cfg.testCmd = "make test";
  cfg.from = "HEAD~3";
  cfg.uncommitted = true;

  IndexHtmlGenerator gen(true, ".", 1, 0, 0, 0, summary, cfg, "", "");
  std::string html = gen.str();

  EXPECT_NE(html.find("HEAD~3"), std::string::npos);
  EXPECT_NE(html.find("uncommitted"), std::string::npos);
}

TEST_F(DocGeneratorTest, testConfigSectionShowsOperators) {
  MutationResults emptyResults;
  MutationSummary summary(emptyResults, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;
  cfg.operators = {"AOR", "ROR", "SDL"};

  IndexHtmlGenerator gen(true, ".", 1, 0, 0, 0, summary, cfg, "", "");
  std::string html = gen.str();

  EXPECT_NE(html.find("operators"), std::string::npos);
  EXPECT_NE(html.find("AOR"), std::string::npos);
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: coverage class thresholds
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testPushItemToTableCoverageThresholdBoundary) {
  // Thresholds: High >= 70, Mid >= 40, Low < 40
  IndexHtmlGenerator genLow(true, ".", 1, 50, 1, 2, 0, "");
  genLow.pushItemToTable("low", 39, 1, 2, 1);
  EXPECT_NE(genLow.str().find("c-lo"), std::string::npos);

  IndexHtmlGenerator genMid(true, ".", 1, 50, 1, 2, 0, "");
  genMid.pushItemToTable("mid", 40, 1, 2, 1);
  EXPECT_NE(genMid.str().find("c-mid"), std::string::npos);

  IndexHtmlGenerator genHigh(true, ".", 1, 50, 1, 2, 0, "");
  genHigh.pushItemToTable("high", 70, 3, 4, 1);
  EXPECT_NE(genHigh.str().find("c-hi"), std::string::npos);
}

TEST_F(DocGeneratorTest, testPushItemToTableCoverageAt69IsMid) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, 0, "");
  gen.pushItemToTable("border", 69, 3, 4, 1);
  std::string html = gen.str();
  EXPECT_NE(html.find("c-mid"), std::string::npos);
  EXPECT_EQ(html.find("c-hi"), std::string::npos);
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: operator bars in summary
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testOperatorBarsShowAllOperators) {
  Mutant m1("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  Mutant m2("SDL", SAMPLE1_PATH, "bar", 2, 1, 2, 5, "{}");
  Mutant m3("ROR", SAMPLE1_PATH, "baz", 3, 1, 3, 5, "==");

  MutationResults results;
  results.push_back(MutationResult(m1, "test1", "", MutationState::KILLED));
  results.push_back(MutationResult(m2, "", "", MutationState::SURVIVED));
  results.push_back(MutationResult(m3, "", "", MutationState::BUILD_FAILURE));

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 3, summary, cfg, "", "");
  std::string html = gen.str();

  EXPECT_NE(html.find("Arithmetic Operator Replacement"), std::string::npos);
  EXPECT_NE(html.find("Statement Deletion"), std::string::npos);
  EXPECT_NE(html.find("Relational Operator Replacement"), std::string::npos);
  EXPECT_NE(html.find("bar-k"), std::string::npos) << "Killed bar segment";
  EXPECT_NE(html.find("bar-s"), std::string::npos) << "Survived bar segment";
  EXPECT_NE(html.find("bar-x"), std::string::npos) << "Skipped bar segment";
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: state timing bars
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testStateTimingBarsPresent) {
  Mutant m1("AOR", SAMPLE1_PATH, "foo", 1, 1, 1, 5, "+");
  Mutant m2("BOR", SAMPLE1_PATH, "bar", 2, 1, 2, 5, "|");

  MutationResults results;
  MutationResult r1(m1, "test1", "", MutationState::KILLED);
  r1.setBuildSecs(10.0);
  r1.setTestSecs(5.0);
  results.push_back(r1);

  MutationResult r2(m2, "", "", MutationState::SURVIVED);
  r2.setBuildSecs(8.0);
  r2.setTestSecs(4.0);
  results.push_back(r2);

  MutationSummary summary(results, SAMPLE1_DIR);
  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2, summary, cfg, "", "");
  std::string html = gen.str();

  EXPECT_NE(html.find("By State"), std::string::npos);
  EXPECT_NE(html.find("bar-build"), std::string::npos);
  EXPECT_NE(html.find("bar-test"), std::string::npos);
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: skipped detail with separators
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testFormatSkippedDetailSeparators) {
  // Verify middot separators between items
  std::string detail = IndexHtmlGenerator::formatSkippedDetail(1, 2, 3);
  // Count occurrences of separator
  std::size_t count = 0;
  std::size_t pos = 0;
  while ((pos = detail.find("&middot;", pos)) != std::string::npos) {
    ++count;
    pos += 8;
  }
  EXPECT_EQ(count, 2u) << "Three items should have 2 separators";
}

TEST_F(DocGeneratorTest, testFormatSkippedDetailNoSeparatorForSingleItem) {
  std::string detail = IndexHtmlGenerator::formatSkippedDetail(5, 0, 0);
  EXPECT_EQ(detail.find("&middot;"), std::string::npos)
      << "Single item should have no separator";
  EXPECT_EQ(detail, "5 timeout");
}

// ---------------------------------------------------------------------------
// IndexHtmlGenerator: zero-duration donut
// ---------------------------------------------------------------------------

TEST_F(DocGeneratorTest, testZeroDurationDonutNotRendered) {
  MutationResults emptyResults;
  MutationSummary summary(emptyResults, SAMPLE1_DIR);

  Config cfg;
  cfg.sourceDir = SAMPLE1_DIR;

  IndexHtmlGenerator gen(true, ".", 0, 0, 0, 0, summary, cfg, "", "");
  std::string html = gen.str();

  // With zero results, no duration donut segments should be generated
  EXPECT_EQ(html.find("stroke=\"#036098\""), std::string::npos)
      << "No build segment for zero duration";
}

}  // namespace sentinel
