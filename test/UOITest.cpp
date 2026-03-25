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

}  // namespace sentinel
