/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COVERAGEINFO_HPP_
#define INCLUDE_SENTINEL_COVERAGEINFO_HPP_

#include <map>
#include <string>
#include <vector>
#include "sentinel/SourceLine.hpp"

namespace sentinel {

/**
 * @brief CoverageInfo class
 */
class CoverageInfo {
 public:
  /**
   * @brief Default constructor
   */
  CoverageInfo() = default;

  /**
   * @brief Constructor
   *
   * @param filenames list of lcov-format coverage result file
   */
  explicit CoverageInfo(const std::vector<std::string>& filenames);

  /**
   * @brief Check if a code line is covered by test cases
   *
   * @param filename source code filename
   * @param line number
   * @return True if line is covered by test cases
   */
  bool cover(const std::string& filename, size_t line);

 private:
  /**
   * @brief map from file name to list of covered lines
   */
  std::map<std::string, std::vector<size_t>> mData;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
