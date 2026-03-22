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

namespace sentinel {

class DocGeneratorTest : public SampleFileGeneratorForTest {};

TEST_F(DocGeneratorTest, testCssGeneratorStrProducesNonEmptyOutput) {
  CssGenerator gen;
  std::string css = gen.str();
  EXPECT_FALSE(css.empty());
  EXPECT_NE(css.find("margin"), std::string::npos);
  EXPECT_NE(css.find("padding"), std::string::npos);
}

TEST_F(DocGeneratorTest, testIndexHtmlGeneratorStrProducesValidHTML) {
  IndexHtmlGenerator gen(true, ".", 1, 50, 1, 2);
  std::string html = gen.str();
  EXPECT_NE(html.find("<html>"), std::string::npos);
  EXPECT_NE(html.find("</html>"), std::string::npos);
}

TEST_F(DocGeneratorTest, testSrcHtmlGeneratorStrProducesValidHTML) {
  SrcHtmlGenerator gen("sample.cpp", true);
  std::string html = gen.str();
  EXPECT_NE(html.find("<html>"), std::string::npos);
  EXPECT_NE(html.find("sample.cpp"), std::string::npos);
}

}  // namespace sentinel
