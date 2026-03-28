/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include <memory>
#include "sentinel/MutationFactory.hpp"

namespace sentinel {

namespace fs = std::filesystem;

MutationFactory::MutationFactory(const std::shared_ptr<MutantGenerator>& generator) : mGenerator(generator) {
}

Mutants MutationFactory::generate(const std::filesystem::path& gitPath, const SourceLines& sourceLines,
                                  std::size_t maxMutants, unsigned int randomSeed) {
  auto mutants = mGenerator->generate(sourceLines, maxMutants, randomSeed);
  const auto root = fs::canonical(gitPath);
  for (auto& m : mutants) {
    auto absPath = fs::canonical(m.getPath());
    m = Mutant(m.getOperator(), absPath.lexically_relative(root), m.getQualifiedFunction(),
               m.getFirst().line, m.getFirst().column,
               m.getLast().line, m.getLast().column, m.getToken());
  }
  return mutants;
}

}  // namespace sentinel
