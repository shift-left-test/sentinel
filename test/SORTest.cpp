/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLine.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

class SORTest : public SampleFileGeneratorForTest {
 protected:
  Mutants populate(int line) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"SOR"});
    MutationFactory factory(generator);
    auto capture = CaptureHelper::getStdoutCapture();
    capture->capture();
    SourceLines lines;
    lines.push_back(SourceLine(SAMPLE1_PATH, line));
    Mutants result = factory.populate(SAMPLE1_DIR, lines, 100, 1234);
    capture->release();
    return result;
  }
};

TEST_F(SORTest, testSORPopulatesOnLeftShift) {
  // Line 58: "    if ((i & 1) == (1 << 0) && i > 0) {"
  // The "<<" in "(1 << 0)" is a left shift — SOR replaces it with ">>".
  Mutants mutants = populate(58);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "SOR");
  }
}

TEST_F(SORTest, testSORSkipsNonShiftOperators) {
  // Line 59: "      ret = ret + i;" — only arithmetic operators, no shifts.
  Mutants mutants = populate(59);
  EXPECT_EQ(mutants.size(), 0u);
}

}  // namespace sentinel
