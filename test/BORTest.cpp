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

}  // namespace sentinel
