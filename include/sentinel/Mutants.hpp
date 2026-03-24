/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANTS_HPP_
#define INCLUDE_SENTINEL_MUTANTS_HPP_

#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "sentinel/Mutant.hpp"

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
   * @brief Load mutants from a file
   *
   * @param path to file
   */
  void load(const std::string& path) {
    Mutant object;
    std::ifstream ifs(path);
    while (ifs >> object) {
      push_back(object);
    }
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANTS_HPP_
