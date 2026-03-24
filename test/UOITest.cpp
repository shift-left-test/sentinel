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

class UOITest : public SampleFileGeneratorForTest {
 protected:
  Mutants generate(int line) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"UOI"});
    MutationFactory factory(generator);
    auto capture = CaptureHelper::getStdoutCapture();
    capture->capture();
    SourceLines lines;
    lines.push_back(SourceLine(SAMPLE1_PATH, line));
    Mutants result = factory.generate(SAMPLE1_DIR, lines, 100, 1234);
    capture->release();
    return result;
  }

  Mutants generateRange(int from, int to) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({"UOI"});
    MutationFactory factory(generator);
    auto capture = CaptureHelper::getStdoutCapture();
    capture->capture();
    SourceLines lines;
    for (int line = from; line <= to; ++line) {
      lines.push_back(SourceLine(SAMPLE1_PATH, line));
    }
    Mutants result = factory.generate(SAMPLE1_DIR, lines, 100, 1234);
    capture->release();
    return result;
  }
};

TEST_F(UOITest, testUOIPopulatesOnIntVariable) {
  // Line 74: "  bool b = i > 0;"
  // UOI can insert "-i" before "i".
  Mutants mutants = generate(74);
  EXPECT_GT(mutants.size(), 0u);
  for (std::size_t i = 0; i < mutants.size(); ++i) {
    EXPECT_EQ(mutants.at(i).getOperator(), "UOI");
  }
}

TEST_F(UOITest, testUOISkipsLambdaCapture) {
  // Lines 82-87: void blockUOIInLambdaCapture() — UOI must not apply inside lambda captures.
  Mutants mutants = generateRange(82, 87);
  EXPECT_EQ(mutants.size(), 0u);
}

TEST_F(UOITest, testUOISkipsMaterializedTemporaryExpr) {
  // Line 100: "  int ret = temporaryBook().num_pages;"
  // MaterializedTemporaryExpr access — UOI must skip this.
  Mutants mutants = generate(100);
  EXPECT_EQ(mutants.size(), 0u);
}

}  // namespace sentinel
