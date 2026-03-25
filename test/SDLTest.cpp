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

TEST_F(SDLTest, testSDLMutatesBreakInForLoop) {
  // Line 28 of sample1b.cpp: "  for (;;) break;  // NOLINT"
  // The "break" is the single-statement body of the for loop — SDL deletes it.
  Mutants mutants = generate(SAMPLE1B_PATH, 28);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "SDL");
  }
}

TEST_F(SDLTest, testSDLSkipsEmptyWhileBody) {
  // Line 30 of sample1b.cpp: "  while (true) {}"
  // Empty loop body — nothing to delete.
  Mutants mutants = generate(SAMPLE1B_PATH, 30);
  EXPECT_EQ(mutants.size(), 0u);
}

}  // namespace sentinel
