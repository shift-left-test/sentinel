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

}  // namespace sentinel
