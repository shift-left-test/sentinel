/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_

#include <yaml-cpp/yaml.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iterator>
#include <sstream>
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
      ofs << "---\n" << data << "\n";
    }
  }

  /**
   * @brief Load results from a file
   *
   * @param path to file
   */
  void load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) {
      return;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (content.empty()) {
      return;
    }
    std::vector<YAML::Node> docs = YAML::LoadAll(content);
    for (const auto& doc : docs) {
      std::ostringstream oss;
      oss << doc;
      std::istringstream iss(oss.str());
      MutationResult obj;
      if (iss >> obj) {
        push_back(obj);
      }
    }
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
