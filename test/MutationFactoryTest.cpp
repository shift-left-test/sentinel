/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

class MutationFactoryTest : public SampleFileGeneratorForTest {
};

TEST_F(MutationFactoryTest, testPopulateWorks) {
  auto mStdoutCapture = CaptureHelper::getStdoutCapture();

  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 58));
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 59));

  std::shared_ptr<MutantGenerator> generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
  MutationFactory factory(generator);

  mStdoutCapture->capture();
  Mutants selected = factory.populate(SAMPLE1_DIR, sourceLines, 3);
  std::string out = mStdoutCapture->release();

  // 1 mutable on line 58, 1 mutable on line 59 are selected
  EXPECT_EQ(selected.size(), 2);
  EXPECT_EQ(selected.at(0).getFirst().line, 58);
  EXPECT_EQ(selected.at(1).getFirst().line, 59);

  EXPECT_TRUE(string::contains(out, SAMPLE1_NAME + "                                                2"));
  EXPECT_TRUE(string::contains(out, "TOTAL                                                      2"));
}

}  // namespace sentinel
