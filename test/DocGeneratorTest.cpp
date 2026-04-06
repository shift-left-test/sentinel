/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
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

}  // namespace sentinel
