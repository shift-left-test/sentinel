/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include "helper/OperatorTestBase.hpp"

namespace sentinel {

namespace fs = std::filesystem;

class AORTest : public OperatorTestBase {
 protected:
  Mutants generate(int line, int limit = 100) {
    return OperatorTestBase::generate("AOR", line, limit);
  }
};

TEST_F(AORTest, testAORPopulatesOnArithmeticExpression) {
  // Line 59: "      ret = ret + i;" — AOR replaces "+" with -, *, /, %
  Mutants mutants = generate(59);
  EXPECT_GT(mutants.size(), 0u);
  bool found = false;
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() == "AOR") found = true;
  }
  EXPECT_TRUE(found);
}

TEST_F(AORTest, testAORSkipsPointerArithmetic) {
  // Line 48: "  return (ptr_end - ptr_start) / sizeof(char);"
  // The "-" between pointers is pointer arithmetic — AOR must skip it.
  Mutants mutants = generate(48);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(AORTest, testAORSkipsMacroExpansion) {
  // Create a source file where the arithmetic operator comes from macro expansion.
  auto tmpDir = testTempDir("SENTINEL_AOR_MACRO_TMP");
  fs::remove_all(tmpDir);
  fs::create_directories(tmpDir);

  auto macroSrc = tmpDir / "macro_test.cpp";
  writeFile(macroSrc, "#define ADD(a, b) ((a) + (b))\nint main() { return ADD(1, 2); }\n");
  writeFile(tmpDir / "compile_commands.json",
      fmt::format("[{{\"directory\": \"{0}\","
                  "\"command\": \"/usr/bin/c++ -o macro_test.o -c {1}\","
                  "\"file\": \"{1}\"}}]",
                  tmpDir.string(), macroSrc.string()));

  auto generator = std::make_shared<UniformMutantGenerator>(tmpDir);
  generator->setOperators({"AOR"});
  MutationFactory factory(generator);
  testing::internal::CaptureStdout();
  SourceLines lines;
  lines.push_back(SourceLine(macroSrc, 2));  // int main() { return ADD(1, 2); }
  Mutants mutants = factory.generate(tmpDir, lines, 100, 1234);
  testing::internal::GetCapturedStdout();

  // AOR must not generate mutants for operators expanded from macros.
  EXPECT_EQ(mutants.size(), 0u);

  fs::remove_all(tmpDir);
}

TEST_F(AORTest, testAORSkipsEquivalentMutantsWithLiteralZeroRHS) {
  // Line 120 of sample1.cpp: "  int a = x + 0;"
  // x + 0 mutations are all equivalent or dangerous (x/0, x%0) — skip all.
  Mutants mutants = generate(120);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() == "AOR") {
      FAIL() << "AOR should skip mutations when RHS is literal 0, but got: "
             << mutants.at(i).getToken();
    }
  }
}

TEST_F(AORTest, testAORSkipsEquivalentMutantsWithLiteralZeroLHS) {
  // Line 123 of sample1.cpp: "  int d = 0 * x;"
  // 0 * x -> 0 / x -> 0 % x are all 0 (equivalent among multiplicative ops).
  // 0 + x and 0 - x differ, so those replacements should be allowed.
  Mutants mutants = generate(123);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() != "AOR") continue;
    std::string token = mutants.at(i).getToken();
    EXPECT_TRUE(token != "/" && token != "%")
        << "AOR should skip multiplicative replacements when LHS is literal 0, but got: "
        << token;
  }
}

TEST_F(AORTest, testAORSkipsEquivalentMutantsWithLiteralOneRHS) {
  // Line 121 of sample1.cpp: "  int b = x * 1;"
  // x * 1 <-> x / 1 are equivalent — skip * <-> / replacement.
  Mutants mutants = generate(121);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    if (mutants.at(i).getOperator() != "AOR") continue;
    // "/" should be filtered (x*1 -> x/1 is equivalent)
    EXPECT_NE(mutants.at(i).getToken(), "/")
        << "AOR should skip * -> / when RHS is literal 1";
  }
}

TEST_F(AORTest, testAORKeepsAddSubReplacementWithLiteralZeroLHS) {
  // Line 123: "  int d = 0 * x;"
  // 0 * x -> 0 + x and 0 - x differ, so + and - should be kept.
  Mutants mutants = generateAll("AOR", 123);
  bool hasAdd = false;
  bool hasSub = false;
  for (const auto& m : mutants) {
    if (m.getOperator() != "AOR") continue;
    if (m.getToken() == "+") hasAdd = true;
    if (m.getToken() == "-") hasSub = true;
  }
  EXPECT_TRUE(hasAdd) << "AOR should keep 0*x -> 0+x";
  EXPECT_TRUE(hasSub) << "AOR should keep 0*x -> 0-x";
}

TEST_F(AORTest, testAORKeepsNonEquivalentMutantsWithLiteralOneRHS) {
  // Line 121 of sample1.cpp: "  int b = x * 1;"
  // x * 1 <-> x / 1 are equivalent — but +, - and % should be kept.
  Mutants mutants = generateAll("AOR", 121);
  bool hasAdd = false;
  bool hasSub = false;
  bool hasMod = false;
  for (const auto& m : mutants) {
    if (m.getOperator() != "AOR") continue;
    if (m.getToken() == "+") hasAdd = true;
    if (m.getToken() == "-") hasSub = true;
    if (m.getToken() == "%") hasMod = true;
  }
  EXPECT_TRUE(hasAdd) << "AOR should keep x*1 -> x+1";
  EXPECT_TRUE(hasSub) << "AOR should keep x*1 -> x-1";
  EXPECT_TRUE(hasMod) << "AOR should keep x*1 -> x%1";
}

TEST_F(AORTest, testAORSkipsPointerArithmeticWithIntArray) {
  // Line 67 of sample1.cpp: "  return (end - start) / sizeof(int);"
  // Pointer subtraction — AOR should not generate mutants here.
  Mutants mutants = generate(67);
  EXPECT_EQ(mutants.size(), 0u);
}

}  // namespace sentinel
