/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_EVALUATOR_HPP_
#define INCLUDE_SENTINEL_EVALUATOR_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"

namespace sentinel {

/**
 * @brief Represents the outcome of building and running tests on a mutant.
 */
enum class TestExecutionState { SUCCESS, BUILD_FAILURE, TIMEOUT, UNCOVERED, RUNTIME_ERROR };

/**
 * @brief Evaluator class
 */
class Evaluator {
 public:
  /**
   * @brief Default constructor
   *
   * @param expectedResultDir Directory Path of Expected Result
   * @throw InvalidArgumentException
   *        when expected result doesn't have passed test case
   */
  explicit Evaluator(const std::filesystem::path& expectedResultDir);
  Evaluator(const Evaluator&) = delete;
  Evaluator& operator=(const Evaluator&) = delete;

  /**
   * @brief Compare an actual with the expected
   *
   * @param mut target mutant
   * @param actualResultDir Directory Path of Actual Result
   * @param testState execution outcome of the mutant
   * @return MutationResult summary of compare
   */
  MutationResult compare(const Mutant& mut, const std::filesystem::path& actualResultDir, TestExecutionState testState);

 private:
  Result mExpectedResult;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EVALUATOR_HPP_
