/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_

#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "sentinel/MutationResult.hpp"

namespace sentinel {

/**
 * @brief Collection of MutationResult objects with file persistence support
 */
class MutationResults : public std::vector<MutationResult> {
 public:
  using std::vector<MutationResult>::vector;

  /**
   * @brief Save the results to a file
   *
   * @param path to file
   */
  void save(const std::string& path) const {
    std::filesystem::path tmpPath(path);
    auto dirname = tmpPath.parent_path();
    if (!dirname.empty()) {
      std::filesystem::create_directories(dirname);
    }
    std::ofstream ofs(path);
    for (const auto& data : *this) {
      ofs << data << "\n";
    }
  }

  /**
   * @brief Load results from a file
   *
   * @param path to file
   */
  void load(const std::string& path) {
    MutationResult object;
    std::ifstream ifs(path);
    while (ifs >> object) {
      push_back(object);
    }
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
