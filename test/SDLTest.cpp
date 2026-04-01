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

}  // namespace sentinel
