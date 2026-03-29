/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_

#include <string>
#include <vector>
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/YamlCollection.hpp"

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
    saveYamlCollection<MutationResult>(*this, path);
  }

  /**
   * @brief Load results from a file
   *
   * @param path to file
   */
  void load(const std::string& path) {
    loadYamlCollection<MutationResult>(this, path);
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
