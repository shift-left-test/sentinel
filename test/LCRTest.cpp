/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

class LCRTest : public OperatorTestBase {
 protected:
  Mutants generate(int line) {
    return OperatorTestBase::generate("LCR", line);
  }
};

TEST_F(LCRTest, testLCRPopulatesOnLogicalAnd) {
  // Line 58: "    if ((i & 1) == (1 << 0) && i > 0) {"
  // The "&&" is a logical AND — LCR should replace it with "||".
  Mutants mutants = generate(58);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "LCR");
  }
}

TEST_F(LCRTest, testLCRSkipsBitwiseOperators) {
  // Line 58 also has "&" (bitwise AND). LCR must not replace "&" — that's BOR's domain.
  // LCR produces: operator replacement (|| or &&) and whole-expression replacement (1 or 0).
  Mutants mutants = generate(58);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    std::string replacement = mutants.at(i).getToken();
    EXPECT_TRUE(replacement == "||" || replacement == "&&" || replacement == "1" || replacement == "0");
  }
}

TEST_F(LCRTest, testLCRSkipsTrueFalseReplacementInLoopCondition) {
  // Line 109 of sample1.cpp: "  while (sum > 0 && n > 0) {"
  // LCR should generate operator replacement (||) but NOT 1/0
  // because the expression is a loop condition.
  Mutants mutants = generateAll("LCR", 109);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() != "LCR") continue;
    EXPECT_NE(mutants.at(i).getToken(), "1");
    EXPECT_NE(mutants.at(i).getToken(), "0");
  }
}

TEST_F(LCRTest, testLCRKeepsTrueFalseReplacementInNonLoopCondition) {
  // Line 58: "    if ((i & 1) == (1 << 0) && i > 0) {"
  // Not a loop condition — LCR should still generate 1/0 replacements.
  Mutants mutants = generateAll("LCR", 58);
  bool hasTrue = false;
  bool hasFalse = false;
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() != "LCR") continue;
    if (mutants.at(i).getToken() == "1") hasTrue = true;
    if (mutants.at(i).getToken() == "0") hasFalse = true;
  }
  EXPECT_TRUE(hasTrue);
  EXPECT_TRUE(hasFalse);
}

}  // namespace sentinel
