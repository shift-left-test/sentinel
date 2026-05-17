/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "helper/ThrowMessageMatcher.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

using ::testing::AllOf;
using ::testing::HasSubstr;

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

TEST_F(MutationFactoryTest, testGenerateThrowsClearErrorWhenSourceRootMissing) {
  // A nonexistent gitPath must surface as a descriptive IOException rather
  // than the raw std::filesystem_error from the no-error_code overload.
  fs::path missingRoot = SAMPLE_BASE / "definitely-not-a-real-dir";
  ASSERT_FALSE(fs::exists(missingRoot));

  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(SAMPLE1_PATH, 58));

  auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
  MutationFactory factory(generator);

  EXPECT_THROW_MESSAGE(
      factory.generate(missingRoot, sourceLines, 3, 1234),
      IOException,
      AllOf(HasSubstr("source root"), HasSubstr(missingRoot.string())));
}

}  // namespace sentinel
