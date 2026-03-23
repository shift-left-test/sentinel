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
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_
