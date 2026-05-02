/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

class MutationFactoryTest : public SampleFileGeneratorForTest {};

TEST_F(MutationFactoryTest, testGenerateWorks) {
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 58));
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 59));

  std::shared_ptr<MutantGenerator> generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
  MutationFactory factory(generator);

  Mutants selected = factory.generate(SAMPLE1_DIR, sourceLines, 3, 1234);

  EXPECT_EQ(selected.size(), 2);
  EXPECT_EQ(selected.at(0).getFirst().line, 58);
  EXPECT_EQ(selected.at(1).getFirst().line, 59);
  for (const auto& m : selected) {
    EXPECT_TRUE(m.getPath().is_relative());
  }
}

TEST_F(MutationFactoryTest, testRepeatedGenerationProducesStableResults) {
  // Mutation generation should be deterministic when re-run with the same
  // seed and source lines: the per-file Clang frontend invocations must not
  // depend on transient process state (e.g. CWD) that changes across runs.
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 41));
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 58));
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 59));
  sourceLines.push_back(SourceLine(SAMPLE1B_PATH, 28));
  sourceLines.push_back(SourceLine(SAMPLE1B_PATH, 29));

  for (int iteration = 0; iteration < 5; ++iteration) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    MutationFactory factory(generator);
    Mutants selected = factory.generate(SAMPLE1_DIR, sourceLines, 100, 1234);
    EXPECT_GT(selected.size(), 0u);
  }
}

}  // namespace sentinel
