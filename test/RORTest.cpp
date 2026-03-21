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

class RORTest : public SampleFileGeneratorForTest {
 protected:
  Mutants populate(int line) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"ROR"});
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

TEST_F(RORTest, testRORPopulatesOnLessEqual) {
  // Line 32: "  return a <= b;"
  Mutants mutants = populate(32);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "ROR");
  }
}

TEST_F(RORTest, testRORPopulatesOnGreaterThan) {
  // Line 41: "  if (d > Friday) {"
  Mutants mutants = populate(41);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "ROR");
  }
}

}  // namespace sentinel
