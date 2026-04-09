/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

UniformMutantGenerator::UniformMutantGenerator(const std::filesystem::path& path) : MutantGenerator(path) {
}

Mutants UniformMutantGenerator::selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                                              unsigned int randomSeed, const CandidateIndex& index,
                                              std::size_t mutantsPerLine) {
  return selectFromRange(sourceLines, maxMutants, randomSeed, index,
                         [](const SourceLine& line) -> const SourceLine& { return line; },
                         mutantsPerLine);
}

}  // namespace sentinel
