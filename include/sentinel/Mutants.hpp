/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANTS_HPP_
#define INCLUDE_SENTINEL_MUTANTS_HPP_

#include <yaml-cpp/yaml.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iterator>
#include <sstream>
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
      ofs << "---\n" << data << "\n";
    }
  }

  /**
   * @brief Load mutants from a file
   *
   * @param path to file
   */
  void load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) return;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (content.empty()) return;
    std::vector<YAML::Node> docs = YAML::LoadAll(content);
    for (const auto& doc : docs) {
      std::ostringstream oss;
      oss << doc;
      std::istringstream iss(oss.str());
      Mutant m;
      if (iss >> m) {
        push_back(m);
      }
    }
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANTS_HPP_
