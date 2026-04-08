/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

class BORTest : public OperatorTestBase {
 protected:
  Mutants generate(int line) {
    return OperatorTestBase::generate("BOR", line);
  }
};

TEST_F(BORTest, testBORPopulatesOnBitwiseAnd) {
  // Line 58: "    if ((i & 1) == (1 << 0) && i > 0) {"
  // The "&" in "(i & 1)" is a bitwise AND — BOR replaces it with | or ^.
  Mutants mutants = generate(58);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "BOR");
  }
}

TEST_F(BORTest, testBORSkipsLogicalOperators) {
  // Line 58 also has "&&". BOR must not replace logical && (that's LCR's domain).
  // We verify by checking that mutants on line 58 are only for bitwise operators.
  Mutants mutants = generate(58);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    // BOR replacements should be &, |, ^ — never &&, ||
    std::string replacement = mutants.at(i).getToken();
    EXPECT_NE(replacement, "&&");
    EXPECT_NE(replacement, "||");
  }
}

TEST_F(BORTest, testBORSkipsEquivalentMutantsWithLiteralZeroRHS) {
  // Line 124 of sample1.cpp: "  int e = x & 0;"
  // x & 0 -> x | 0 -> x ^ 0 are all constant/identity — skip.
  Mutants mutants = generate(124);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() == "BOR") {
      FAIL() << "BOR should skip mutations when RHS is literal 0, but got: "
             << mutants.at(i).getToken();
    }
  }
}

TEST_F(BORTest, testBORSkipsEquivalentMutantsWithLiteralZeroRHS2) {
  // Line 125 of sample1.cpp: "  int f = x | 0;"
  // x | 0 -> x & 0 -> x ^ 0 are all constant/identity — skip.
  Mutants mutants = generate(125);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() == "BOR") {
      FAIL() << "BOR should skip mutations when RHS is literal 0, but got: "
             << mutants.at(i).getToken();
    }
  }
}

TEST_F(BORTest, testBORSkipsEquivalentMutantsWithLiteralZeroLHS) {
  // Line 127 of sample1.cpp: "  int h = 0 >> x;"
  // 0 >> x is always 0, so 0 & x, 0 | x, 0 ^ x are all constant/identity — skip.
  Mutants mutants = generate(127);
  for (const auto& m : mutants) {
    if (m.getOperator() == "BOR") {
      FAIL() << "BOR should skip mutations when LHS is literal 0, but got: "
             << m.getToken();
    }
  }
}

TEST_F(BORTest, testBORDoesNotTargetShiftOperators) {
  // Line 128 of sample1.cpp: "  int j = x << 2;"
  // BOR only targets &, |, ^ — not shift operators (SOR handles those).
  Mutants mutants = generate(128);
  for (const auto& m : mutants) {
    EXPECT_NE(m.getOperator(), "BOR")
        << "BOR should not target shift operators";
  }
}

}  // namespace sentinel
