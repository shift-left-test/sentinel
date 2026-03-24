/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_
#define INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_

#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"

namespace sentinel {

/**
 * @brief MutantGenerator class
 */
class MutantGenerator {
 public:
  /**
   * @brief Default destructor
   */
  virtual ~MutantGenerator() = default;

  /**
   * @brief Generate mutables from the given source line
   *
   * @param sourceLines list of target source lines
   * @param maxMutants limit number of generated mutables
   * @param randomSeed random seed
   * @return mutables
   */
  virtual Mutants generate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned randomSeed) = 0;

  /**
   * @brief Set mutation operators to use. If empty, all operators are used.
   *
   * @param ops list of operator names (e.g. "AOR", "BOR")
   */
  void setOperators(const std::vector<std::string>& ops) {
    mSelectedOperators = ops;
  }

  /**
   * @brief Return a new mutant generator instance based on the specified options
   *
   * @param generator name
   * @param directory path to the directory containing the compile_commands.json file
   * @return mutant generator instance
   */
  static std::shared_ptr<MutantGenerator> getInstance(const std::string& generator,
                                                      const std::filesystem::path& directory);

  /**
   * @brief Return the total number of candidate mutants found before the limit was applied.
   *
   * @return candidate count from the last generate() call
   */
  std::size_t getCandidateCount() const {
    return mCandidateCount;
  }

 protected:
  /**
   * @brief list of operator names to use (empty means all operators)
   */
  std::vector<std::string> mSelectedOperators;

  /**
   * @brief total candidates found before limit was applied; set by each generate() implementation
   */
  std::size_t mCandidateCount = 0;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_
