/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_EVALUATOR_HPP_
#define INCLUDE_SENTINEL_EVALUATOR_HPP_

#include <experimental/filesystem>
#include <memory>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Result.hpp"

namespace sentinel {

/**
 * @brief Evaluator class
 */
class Evaluator {
 public:
  /**
   * @brief Default constructor
   *
   * @param expectedResultDir Directory Path of Expected Result
   * @param sourcePath Directory Path of Source
   * @throw InvalidArgumentException
   *        when expected result doesn't have passed test case
   */
  explicit Evaluator(const std::string& expectedResultDir, const std::string& sourcePath);

  /**
   * @brief Compare an actual with the expected
   *
   * @param mut taget mutable
   * @param ActualResultDir Directory Path of Actural Result
   * @param testState one of ['success', 'build_failure', 'timeout']
   * @return MutationResult summary of compare
   */
  MutationResult compare(const Mutant& mut, const std::string& ActualResultDir, const std::string& testState);

  /**
   * @brief Compare an actual with the expected and save&return summary
   *
   * @param mut taget mutable
   * @param ActualResultDir Directory Path of Actural Result
   * @param evalFilePath File Path of MutationResult
   * @param testState one of ['success', 'build_failure', 'timeout']
   * @return MutationResult summary of compare
   * @throw InvalidArgumentException
   *        when evalFile's parent's path isn't directory
   */
  MutationResult compareAndSaveMutationResult(const Mutant& mut,
                                              const std::experimental::filesystem::path& ActualResultDir,
                                              const std::experimental::filesystem::path& evalFilePath,
                                              const std::string& testState);

  /**
   * @brief Return mutation results
   *
   * @return MutationResults
   */
  const MutationResults& getMutationResults() { return mMutationResults; }

 private:
  std::shared_ptr<Logger> mLogger;
  std::string mSourcePath;
  Mutants mMutants;
  Result mExpectedResult;
  MutationResults mMutationResults;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EVALUATOR_HPP_
