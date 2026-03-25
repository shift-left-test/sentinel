/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include <memory>
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLines.hpp"

namespace sentinel {

MutationFactory::MutationFactory(const std::shared_ptr<MutantGenerator>& generator) : mGenerator(generator) {
}

Mutants MutationFactory::generate(const std::filesystem::path& gitPath, const SourceLines& sourceLines,
                                  std::size_t maxMutants, unsigned int randomSeed) {
  return mGenerator->generate(sourceLines, maxMutants, randomSeed);
}

}  // namespace sentinel
