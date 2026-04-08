/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <set>
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

TEST_F(RORTest, testRORGeneratesOperatorReplacementsForEnum) {
  // Line 41: "if (d > Friday)" — d is an enum, not nullptr. Check that
  // ROR generates operator replacements for non-nullptr operands.
  Mutants mutants = generateAll("ROR", 41);
  EXPECT_GT(mutants.size(), 0u);
  bool hasOperatorReplacement = std::any_of(
      mutants.begin(), mutants.end(), [](const auto& m) {
        return m.getOperator() == "ROR" && m.getToken() != "1" &&
               m.getToken() != "0";
      });
  EXPECT_TRUE(hasOperatorReplacement)
      << "ROR should generate operator replacements for non-nullptr operand";
}

TEST_F(RORTest, testRORSkipsTrueFalseReplacementInWhileCondition) {
  // Line 109 of sample1.cpp: "  while (sum > 0 && n > 0) {"
  // ROR on "> 0" inside while should not produce 1/0.
  Mutants mutants = generateAll("ROR", 109);
  for (const auto& m : mutants) {
    if (m.getOperator() != "ROR") continue;
    EXPECT_NE(m.getToken(), "1")
        << "ROR should not produce true(1) in while loop condition";
    EXPECT_NE(m.getToken(), "0")
        << "ROR should not produce false(0) in while loop condition";
  }
}

TEST_F(RORTest, testRORGeneratesMultipleOperatorReplacements) {
  // Line 32: "return a <= b;" — non-loop condition
  // ROR should replace <= with at least some of <, >, >=, ==, !=
  // plus true(1) and false(0) since it's not in a loop.
  Mutants mutants = generateAll("ROR", 32);
  EXPECT_GE(mutants.size(), 3u);
  std::set<std::string> tokens;
  for (const auto& m : mutants) {
    tokens.insert(m.getToken());
  }
  EXPECT_TRUE(tokens.count("1") > 0) << "Should have true replacement";
  EXPECT_TRUE(tokens.count("0") > 0) << "Should have false replacement";
  EXPECT_GE(tokens.size(), 3u) << "Should have multiple distinct replacements";
}

}  // namespace sentinel
