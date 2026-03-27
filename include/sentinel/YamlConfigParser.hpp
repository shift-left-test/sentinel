/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_
#define INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Config.hpp"

namespace sentinel {

/**
 * @brief Parses a YAML config file and applies values to a Config object.
 */
class YamlConfigParser {
 public:
  /**
   * @brief Apply values from a YAML file onto an existing Config.
   *
   * Only fields present in the YAML file are overwritten.
   * Path fields are resolved relative to the YAML file's directory.
   *
   * @param cfg    Config to modify in place.
   * @param path   Path to the YAML config file.
   */
  static void applyTo(Config* cfg, const std::filesystem::path& path);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_YAMLCONFIGPARSER_HPP_
