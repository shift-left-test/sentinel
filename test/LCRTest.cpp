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

}  // namespace sentinel
