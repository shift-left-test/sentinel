/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

class SORTest : public OperatorTestBase {
 protected:
  Mutants generate(int line) {
    return OperatorTestBase::generate("SOR", line);
  }
};

TEST_F(SORTest, testSORPopulatesOnLeftShift) {
  // Line 128 of sample1.cpp: "  int j = x << 2;"
  // Non-literal-0 shift — SOR should still generate mutation (>> replacement).
  Mutants mutants = generate(128);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "SOR");
  }
}

TEST_F(SORTest, testSORSkipsNonShiftOperators) {
  // Line 59: "      ret = ret + i;" — only arithmetic operators, no shifts.
  Mutants mutants = generate(59);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(SORTest, testSORSkipsEquivalentMutantsWithLiteralZero) {
  // Line 126 of sample1.cpp: "  int g = x << 0;"
  // x << 0 <-> x >> 0 both produce x — equivalent mutant.
  Mutants mutants = generate(126);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() == "SOR") {
      FAIL() << "SOR should skip mutations when operand is literal 0, but got: "
             << mutants.at(i).getToken();
    }
  }
}

TEST_F(SORTest, testSORSkipsEquivalentMutantsWithLiteralZeroLHS) {
  // Line 127 of sample1.cpp: "  int h = 0 >> x;"
  // 0 >> x <-> 0 << x both produce 0 — equivalent mutant.
  Mutants mutants = generate(127);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() == "SOR") {
      FAIL() << "SOR should skip mutations when LHS is literal 0, but got: "
             << mutants.at(i).getToken();
    }
  }
}

}  // namespace sentinel
