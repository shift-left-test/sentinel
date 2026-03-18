/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONFIGRESOLVER_HPP_
#define INCLUDE_SENTINEL_CONFIGRESOLVER_HPP_

#include <string>
#include "sentinel/Config.hpp"

namespace sentinel {

/**
 * @brief Merges multiple configuration sources and resolves paths/defaults.
 */
class ConfigResolver {
 public:
  /**
   * @brief Merges CLI, YAML and default values into a final configuration.
   *
   * @param cli Config derived from CLI arguments.
   * @param yaml Config derived from sentinel.yaml.
   * @param yamlPath Path to the sentinel.yaml file (used for relative path resolution).
   * @return A finalized Config object with all paths absolute and defaults filled.
   */
  static Config resolve(const Config& cli, const Config& yaml, const std::string& yamlPath = "");

 private:
  /**
   * @brief Applies priority (CLI > YAML > Default) and fills a target field.
   */
  template <typename T>
  static void mergeField(std::optional<T>& target, const std::optional<T>& cli,
                         const std::optional<T>& yaml, const T& defaultValue) {
    if (cli.has_value()) {
      target = cli;
    } else if (yaml.has_value()) {
      target = yaml;
    } else {
      target = defaultValue;
    }
  }

  /**
   * @brief Merges fields without a default value (leaves as std::nullopt if both sources empty).
   */
  template <typename T>
  static void mergeFieldOptional(std::optional<T>& target, const std::optional<T>& cli,
                                 const std::optional<T>& yaml) {
    if (cli.has_value()) {
      target = cli;
    } else {
      target = yaml;
    }
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CONFIGRESOLVER_HPP_
