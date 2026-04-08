/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

class SDLTest : public OperatorTestBase {
 protected:
  Mutants generate(const std::filesystem::path& srcPath, int line) {
    return OperatorTestBase::generate("SDL", srcPath, line);
  }
};

TEST_F(SDLTest, testSDLPopulatesOnSimpleStatement) {
  // Line 61: "    i++;" — a simple expression statement that can be deleted.
  Mutants mutants = generate(SAMPLE1_PATH, 61);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "SDL");
  }
}

TEST_F(SDLTest, testSDLSkipsBreakInForLoop) {
  // Line 28 of sample1b.cpp: "  for (;;) break;  // NOLINT"
  // Deleting break would cause an infinite loop — SDL must skip it.
  Mutants mutants = generate(SAMPLE1B_PATH, 28);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(SDLTest, testSDLSkipsContinueInWhileLoop) {
  // Line 31 of sample1b.cpp: "  while (true) continue;"
  // Deleting continue would cause an infinite loop — SDL must skip it.
  Mutants mutants = generate(SAMPLE1B_PATH, 31);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(SDLTest, testSDLSkipsEmptyWhileBody) {
  // Line 30 of sample1b.cpp: "  while (true) {}"
  // Empty loop body — nothing to delete.
  Mutants mutants = generate(SAMPLE1B_PATH, 30);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(SDLTest, testSDLDeleteTokenIsEmptyBlock) {
  // Line 61 of sample1.cpp: "    i++;" — SDL replaces with "{}".
  Mutants mutants = generate(SAMPLE1_PATH, 61);
  EXPECT_GT(mutants.size(), 0u);
  for (const auto& m : mutants) {
    EXPECT_EQ(m.getOperator(), "SDL");
    EXPECT_EQ(m.getToken(), "{}");
  }
}

TEST_F(SDLTest, testSDLSkipsReturnStatement) {
  // Line 64 of sample1.cpp: "  return ret;" — SDL should skip return statements.
  Mutants mutants = generate(SAMPLE1_PATH, 64);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(SDLTest, testSDLSkipsDeclarationStatement) {
  // Line 54 of sample1.cpp: "  int ret = 0;" — DeclStmt should be skipped.
  Mutants mutants = generate(SAMPLE1_PATH, 54);
  for (const auto& m : mutants) {
    EXPECT_NE(m.getOperator(), "SDL")
        << "SDL should skip declaration statements";
  }
}

TEST_F(SDLTest, testSDLSkipsDeleteExpression) {
  // Line 36 of sample1b.cpp: "  delete ptr;" — CXXDeleteExpr should be skipped.
  Mutants mutants = generate(SAMPLE1B_PATH, 36);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(SDLTest, testSDLSkipsLastStatementOfStmtExpr) {
  // Line 32 of sample1b.cpp: "  return ({3;});" — the "3;" is the value of
  // the Statement Expression, SDL should not delete it.
  Mutants mutants = generate(SAMPLE1B_PATH, 32);
  EXPECT_EQ(mutants.size(), 0u);
}

}  // namespace sentinel
