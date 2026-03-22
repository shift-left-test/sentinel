/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_
#define INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Config.hpp"

namespace sentinel {

/**
 * @brief Parses sentinel.yaml and populates a Config object.
 */
class YamlConfigParser {
 public:
  /**
   * @brief Load configuration from a YAML file.
   *
   * @param path Path to the YAML configuration file.
   * @return Parsed Config object.
   */
  static Config loadFromFile(const std::filesystem::path& path);

  /**
   * @brief Write a sentinel.yaml template to the given path.
   *
   * Always overwrites. Caller is responsible for any overwrite check.
   *
   * @param path Destination file path.
   * @throws std::runtime_error if the file cannot be opened for writing.
   */
  static void writeTemplate(const std::filesystem::path& path);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_
