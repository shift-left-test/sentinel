/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

class UOITest : public OperatorTestBase {
 protected:
  Mutants generate(int line) {
    return OperatorTestBase::generate("UOI", line);
  }

  Mutants generateRange(int fromLine, int toLine) {
    return OperatorTestBase::generateRange("UOI", fromLine, toLine);
  }
};

TEST_F(UOITest, testUOIPopulatesOnIntVariable) {
  // Line 74: "  bool b = i > 0;"
  // UOI can insert "-i" before "i".
  Mutants mutants = generate(74);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "UOI");
  }
}

TEST_F(UOITest, testUOISkipsLambdaCapture) {
  // Lines 82-87: void blockUOIInLambdaCapture() — UOI must not apply inside lambda captures.
  Mutants mutants = generateRange(82, 87);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(UOITest, testUOISkipsMaterializedTemporaryExpr) {
  // Line 100: "  int ret = temporaryBook().num_pages;"
  // MaterializedTemporaryExpr access — UOI must skip this.
  Mutants mutants = generate(100);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(UOITest, testUOISkipsIncrementDecrementInLoopCondition) {
  // Line 109 of sample1.cpp: "  while (sum > 0 && n > 0) {"
  // UOI must not insert ++(sum) or --(sum) in loop condition — side effects alter loop control.
  // Boolean negation !(bool) would be OK but sum/n are int, not bool.
  Mutants mutants = generate(109);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() != "UOI") continue;
    // No UOI mutants should exist for variables in this loop condition
    FAIL() << "UOI should not generate mutants in loop condition, but got: "
           << mutants.at(i).getToken();
  }
}

}  // namespace sentinel
