/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
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
            MutationOperatorToExpansion("AOR"));
  EXPECT_EQ("UOI (Unary Operator Insertion)",
            MutationOperatorToExpansion("UOI"));
}

}  // namespace sentinel
