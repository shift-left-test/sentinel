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
  // Line 58: "    if ((i & 1) == (1 << 0) && i > 0) {"
  // The "<<" in "(1 << 0)" is a left shift — SOR replaces it with ">>".
  Mutants mutants = generate(58);
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

}  // namespace sentinel
