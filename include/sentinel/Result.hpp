/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_RESULT_HPP_
#define INCLUDE_SENTINEL_RESULT_HPP_

#include <memory>
#include <string>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/MutationState.hpp"


namespace sentinel {

/**
 * @brief Result class
 */
class Result {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to a test result
   */
  explicit Result(const std::string& path);

  /**
   * @brief check if passed testcase doesn't exist
   *
   * @return true if number of passed Testcase is 0
   */
  bool checkPassedTCEmpty();

  /**
   * @brief Check mutation's Result State
   *
   * @param original result
   * @param mutated result
   * @param [out] killingTest
   * @param [out] errorTest
   * @return mutation's Result State
   */
  static MutationState compare(const Result& original, const Result& mutated,
      std::string* killingTest, std::string* errorTest);

 private:
  std::vector<std::string> mPassedTC;
  std::vector<std::string> mFailedTC;
  std::shared_ptr<Logger> mLogger;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RESULT_HPP_
