/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONFACTORY_HPP_
#define INCLUDE_SENTINEL_MUTATIONFACTORY_HPP_

#include <memory>
#include <string>
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"


namespace sentinel {

/**
 * @brief MutationFactory class
 */
class MutationFactory {
 public:
  /**
   * @brief Default constructor
   *
   * @param generator Mutation generator
   */
  explicit MutationFactory(const std::shared_ptr<MutantGenerator>& generator);

  /**
   * @brief Populate mutables from the given source lines
   *
   * @param gitPath path to git repo
   * @param sourceLines lines of the source
   * @param maxMutants maximum number of mutables generated
   * @param randomSeed random seed
   * @return list of mutables
   */
  Mutants populate(const std::string& gitPath,
                   const SourceLines& sourceLines,
                   std::size_t maxMutants,
                   unsigned randomSeed);

  /**
   * @brief Populate mutables from the given source lines.
   *
   * @param gitPath path to git repo
   * @param sourceLines lines of the source
   * @param maxMutants maximum number of mutables generated
   * @return list of mutables
   */
  Mutants populate(const std::string& gitPath,
                   const SourceLines& sourceLines,
                   std::size_t maxMutants);

 private:
  std::shared_ptr<MutantGenerator> mGenerator;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONFACTORY_HPP_
