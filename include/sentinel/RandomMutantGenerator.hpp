/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_
#define INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/MutantGenerator.hpp"

namespace sentinel {

/**
 * @brief Random mutant generator — collects all candidates then applies Fisher-Yates sampling.
 */
class RandomMutantGenerator : public MutantGenerator {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to compilation database file
   */
  explicit RandomMutantGenerator(const std::filesystem::path& path);

 protected:
  Mutants selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                        unsigned int randomSeed, const CandidateIndex& index,
                        std::size_t mutantsPerLine) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_
