/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/docGenerator/CSSGenerator.hpp"
#include "sentinel/docGenerator/IndexHTMLGenerator.hpp"
#include "sentinel/docGenerator/SrcHTMLGenerator.hpp"

namespace sentinel {

class DocGeneratorTest : public SampleFileGeneratorForTest {};

TEST_F(DocGeneratorTest, testCSSGeneratorStrProducesNonEmptyOutput) {
  CSSGenerator gen;
  std::string css = gen.str();
  EXPECT_FALSE(css.empty());
  EXPECT_NE(css.find("margin"), std::string::npos);
  EXPECT_NE(css.find("padding"), std::string::npos);
}

TEST_F(DocGeneratorTest, testIndexHTMLGeneratorStrProducesValidHTML) {
  IndexHTMLGenerator gen(true, ".", 1, 50, 1, 2);
  std::string html = gen.str();
  EXPECT_NE(html.find("<html>"), std::string::npos);
  EXPECT_NE(html.find("</html>"), std::string::npos);
}

TEST_F(DocGeneratorTest, testSrcHTMLGeneratorStrProducesValidHTML) {
  SrcHTMLGenerator gen("sample.cpp", true);
  std::string html = gen.str();
  EXPECT_NE(html.find("<html>"), std::string::npos);
  EXPECT_NE(html.find("sample.cpp"), std::string::npos);
}

}  // namespace sentinel
