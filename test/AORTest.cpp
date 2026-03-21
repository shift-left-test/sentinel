/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLine.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

namespace fs = std::filesystem;

class AORTest : public SampleFileGeneratorForTest {
 protected:
  Mutants populate(int line, int limit = 100) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"AOR"});
    MutationFactory factory(generator);
    auto capture = CaptureHelper::getStdoutCapture();
    capture->capture();
    SourceLines lines;
    lines.push_back(SourceLine(SAMPLE1_PATH, line));
    Mutants result = factory.populate(SAMPLE1_DIR, lines, limit, 1234);
    capture->release();
    return result;
  }
};

TEST_F(AORTest, testAORPopulatesOnArithmeticExpression) {
  // Line 59: "      ret = ret + i;" — AOR replaces "+" with -, *, /, %
  Mutants mutants = populate(59);
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
  Mutants mutants = populate(48);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(AORTest, testAORSkipsMacroExpansion) {
  // Create a source file where the arithmetic operator comes from macro expansion.
  auto tmpDir = fs::temp_directory_path() / "SENTINEL_AOR_MACRO_TMP";
  fs::remove_all(tmpDir);
  fs::create_directories(tmpDir);

  auto macroSrc = tmpDir / "macro_test.cpp";
  {
    std::ofstream f(macroSrc);
    f << "#define ADD(a, b) ((a) + (b))\nint main() { return ADD(1, 2); }\n";
  }
  {
    std::ofstream f(tmpDir / "compile_commands.json");
    f << "[{\"directory\": \"" << tmpDir.string() << "\","
      << "\"command\": \"/usr/bin/c++ -o macro_test.o -c " << macroSrc.string() << "\","
      << "\"file\": \"" << macroSrc.string() << "\"}]";
  }

  auto generator = std::make_shared<UniformMutantGenerator>(tmpDir);
  generator->setOperators({"AOR"});
  MutationFactory factory(generator);
  auto capture = CaptureHelper::getStdoutCapture();
  capture->capture();
  SourceLines lines;
  lines.push_back(SourceLine(macroSrc, 2));  // int main() { return ADD(1, 2); }
  Mutants mutants = factory.populate(tmpDir, lines, 100, 1234);
  capture->release();

  // AOR must not generate mutants for operators expanded from macros.
  EXPECT_EQ(mutants.size(), 0u);

  fs::remove_all(tmpDir);
}

}  // namespace sentinel
