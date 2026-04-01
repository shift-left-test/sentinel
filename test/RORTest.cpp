/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

class RORTest : public OperatorTestBase {
 protected:
  Mutants generate(int line) {
    return OperatorTestBase::generate("ROR", line);
  }
};

TEST_F(RORTest, testRORPopulatesOnLessEqual) {
  // Line 32: "  return a <= b;"
  Mutants mutants = generate(32);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "ROR");
  }
}

TEST_F(RORTest, testRORPopulatesOnGreaterThan) {
  // Line 41: "  if (d > Friday) {"
  Mutants mutants = generate(41);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "ROR");
  }
}

TEST_F(RORTest, testRORSkipsTrueFalseReplacementInLoopCondition) {
  // Line 106 of sample1.cpp: "  for (int i = 0; i < n; i++) {"
  // ROR should generate operator replacements (<=, >, >=, ==, !=) but NOT 1/0
  // because the expression is a loop condition and 1 would cause infinite loop.
  Mutants mutants = generateAll("ROR", 106);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "ROR");
    EXPECT_NE(mutants.at(i).getToken(), "1");
    EXPECT_NE(mutants.at(i).getToken(), "0");
  }
}

TEST_F(RORTest, testRORSkipsTrueFalseReplacementInDoWhileCondition) {
  // Line 115 of sample1.cpp: "  } while (sum < 0);"
  // ROR should generate operator replacements but NOT 1/0 in do-while condition.
  Mutants mutants = generateAll("ROR", 115);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "ROR");
    EXPECT_NE(mutants.at(i).getToken(), "1");
    EXPECT_NE(mutants.at(i).getToken(), "0");
  }
}

TEST_F(RORTest, testRORKeepsTrueFalseReplacementInNonLoopCondition) {
  // Line 32: "  return a <= b;" — not a loop condition.
  // ROR should still generate 1/0 replacements here.
  Mutants mutants = generateAll("ROR", 32);
  bool hasTrue = false;
  bool hasFalse = false;
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getToken() == "1") hasTrue = true;
    if (mutants.at(i).getToken() == "0") hasFalse = true;
  }
  EXPECT_TRUE(hasTrue);
  EXPECT_TRUE(hasFalse);
}

}  // namespace sentinel
