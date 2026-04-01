/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_HELPER_OPERATORTESTBASE_HPP_
#define TEST_INCLUDE_HELPER_OPERATORTESTBASE_HPP_

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLine.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

/**
 * @brief A generator that returns all candidates without selection or limit.
 */
class AllMutantGenerator : public UniformMutantGenerator {
 public:
  using UniformMutantGenerator::UniformMutantGenerator;

 protected:
  Mutants selectMutants(const SourceLines& sourceLines, std::size_t /*maxMutants*/,
                        unsigned int /*randomSeed*/, const CandidateIndex& index) override {
    return index.allMutants;
  }
};

/**
 * @brief Base fixture for mutation operator tests (AOR, BOR, LCR, etc.).
 *
 * Provides shared generate() and generateRange() helpers that suppress
 * stdout during Clang AST parsing and return the resulting Mutants.
 */
class OperatorTestBase : public SampleFileGeneratorForTest {
 protected:
  static constexpr int kTestSeed = 1234;

  Mutants generate(const std::string& op, int line, int limit = 100) {
    return generate(op, SAMPLE1_PATH, line, limit);
  }

  Mutants generate(const std::string& op, const std::filesystem::path& srcPath,
                   int line, int limit = 100) {
    SourceLines lines;
    lines.push_back(SourceLine(srcPath, line));
    return runGenerate(op, lines, limit);
  }

  Mutants generateAll(const std::string& op, int line) {
    SourceLines lines;
    lines.push_back(SourceLine(SAMPLE1_PATH, line));
    return runGenerateAll(op, lines);
  }

  Mutants generateRange(const std::string& op, int fromLine, int toLine, int limit = 100) {
    SourceLines lines;
    lines.reserve(toLine - fromLine + 1);
    for (int line = fromLine; line <= toLine; ++line) {
      lines.push_back(SourceLine(SAMPLE1_PATH, line));
    }
    return runGenerate(op, lines, limit);
  }

 private:
  Mutants runGenerate(const std::string& op, const SourceLines& lines, int limit) {
    auto generator = std::make_shared<UniformMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({op});
    MutationFactory factory(generator);
    testing::internal::CaptureStdout();
    Mutants result = factory.generate(SAMPLE1_DIR, lines, limit, kTestSeed);
    testing::internal::GetCapturedStdout();
    return result;
  }

  Mutants runGenerateAll(const std::string& op, const SourceLines& lines) {
    auto generator = std::make_shared<AllMutantGenerator>(SAMPLE1_DIR);
    generator->setOperators({op});
    MutationFactory factory(generator);
    testing::internal::CaptureStdout();
    Mutants result = factory.generate(SAMPLE1_DIR, lines, 0, kTestSeed);
    testing::internal::GetCapturedStdout();
    return result;
  }
};

}  // namespace sentinel

#endif  // TEST_INCLUDE_HELPER_OPERATORTESTBASE_HPP_
