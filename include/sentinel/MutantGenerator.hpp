/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_
#define INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_

#include <string>
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"


namespace sentinel {

/**
 * @brief MutantGenerator class
 */
class MutantGenerator {
 public:
  /**
   * @brief Populate mutables from the given source line
   *
   * @param sourceLines list of target source lines
   * @param maxMutants limit number of generated mutables
   * @param randomSeed random seed
   * @return mutables
   */
  virtual Mutants populate(const SourceLines& sourceLines,
                           std::size_t maxMutants,
                           unsigned randomSeed) = 0;

  /**
   * @brief Populate mutables from the given source line
   *
   * @param sourceLines list of target source lines
   * @param maxMutants limit number of generated mutables
   * @return mutables
   */
  virtual Mutants populate(const SourceLines& sourceLines,
                           std::size_t maxMutants) = 0;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_
