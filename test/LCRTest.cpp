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

class LCRTest : public SampleFileGeneratorForTest {
 protected:
  Mutants generate(int line) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"LCR"});
    MutationFactory factory(generator);
    auto capture = CaptureHelper::getStdoutCapture();
    capture->capture();
    SourceLines lines;
    lines.push_back(SourceLine(SAMPLE1_PATH, line));
    Mutants result = factory.generate(SAMPLE1_DIR, lines, 100, 1234);
    capture->release();
    return result;
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
