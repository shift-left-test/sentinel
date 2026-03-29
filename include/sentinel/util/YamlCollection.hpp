/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_YAMLCOLLECTION_HPP_
#define INCLUDE_SENTINEL_UTIL_YAMLCOLLECTION_HPP_

#include <yaml-cpp/yaml.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace sentinel {

/**
 * @brief Save a collection of YAML-serializable items to a file
 *
 * @tparam T element type (must support operator<<)
 * @param items collection to save
 * @param path output file path
 */
template <typename T>
void saveYamlCollection(const std::vector<T>& items, const std::string& path) {
  std::filesystem::path tmpPath(path);
  auto dirname = tmpPath.parent_path();
  if (!dirname.empty()) {
    std::filesystem::create_directories(dirname);
  }
  std::ofstream ofs(path);
  for (const auto& data : items) {
    ofs << "---\n" << data << "\n";
  }
}

/**
 * @brief Load a collection of YAML-serializable items from a file
 *
 * @tparam T element type (must support operator>>)
 * @param items output collection to populate
 * @param path input file path
 */
template <typename T>
void loadYamlCollection(std::vector<T>* items, const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) return;
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  if (content.empty()) return;
  std::vector<YAML::Node> docs = YAML::LoadAll(content);
  for (const auto& doc : docs) {
    std::ostringstream oss;
    oss << doc;
    std::istringstream iss(oss.str());
    T obj;
    if (iss >> obj) {
      items->push_back(obj);
    }
  }
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_YAMLCOLLECTION_HPP_
