/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLine.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

class SDLTest : public SampleFileGeneratorForTest {
 protected:
  Mutants generate(const std::filesystem::path& srcPath, int line) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"SDL"});
    MutationFactory factory(generator);
    auto capture = CaptureHelper::getStdoutCapture();
    capture->capture();
    SourceLines lines;
    lines.push_back(SourceLine(srcPath, line));
    Mutants result = factory.generate(SAMPLE1_DIR, lines, 100, 1234);
    capture->release();
    return result;
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
