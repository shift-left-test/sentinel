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

class BORTest : public SampleFileGeneratorForTest {
 protected:
  Mutants generate(int line) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"BOR"});
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
