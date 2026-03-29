/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANTS_HPP_
#define INCLUDE_SENTINEL_MUTANTS_HPP_

#include <string>
#include <vector>
#include "sentinel/Mutant.hpp"
#include "sentinel/util/YamlCollection.hpp"

namespace sentinel {

/**
 * @brief Collection of Mutant objects with file persistence support
 */
class Mutants : public std::vector<Mutant> {
 public:
  using std::vector<Mutant>::vector;

  /**
   * @brief Save the mutants to a file
   *
   * @param path to file
   */
  void save(const std::string& path) const {
    saveYamlCollection<Mutant>(*this, path);
  }

  /**
   * @brief Load mutants from a file
   *
   * @param path to file
   */
  void load(const std::string& path) {
    loadYamlCollection<Mutant>(this, path);
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANTS_HPP_
