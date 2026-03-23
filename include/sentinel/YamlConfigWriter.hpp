/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_YAMLCONFIGWRITER_HPP_
#define INCLUDE_SENTINEL_YAMLCONFIGWRITER_HPP_

#include <filesystem>  // NOLINT

namespace sentinel {

/**
 * @brief Writes a sentinel.yaml configuration template to disk.
 */
class YamlConfigWriter {
 public:
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

#endif  // INCLUDE_SENTINEL_YAMLCONFIGWRITER_HPP_
