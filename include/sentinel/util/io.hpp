/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_IO_HPP_
#define INCLUDE_SENTINEL_UTIL_IO_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/formatter.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel::io {

/**
 * @brief Check whether a path has a .xml extension (case-insensitive).
 *
 * @param p Path to check.
 * @return true if the extension is .xml (any case).
 */
inline bool isXmlFile(const std::filesystem::path& p) {
  return string::toLower(p.extension().string()) == ".xml";
}

/**
 * @brief Ensure the given path is a directory, creating it if it does not exist.
 *
 * @param dirPath path to ensure as a directory
 * @throw InvalidArgumentException if the path exists but is not a directory
 */
inline void ensureDirectoryExists(const std::filesystem::path& dirPath) {
  try {
    if (!std::filesystem::create_directories(dirPath) && !std::filesystem::is_directory(dirPath)) {
      throw InvalidArgumentException(fmt::format("'{}' is not a directory", dirPath));
    }
  } catch (const std::filesystem::filesystem_error&) {
    throw InvalidArgumentException(fmt::format("'{}' is not a directory", dirPath));
  }
}

/**
 * @brief Synchronize @p to with XML files from @p from.
 *        Replaces the entire contents of @p to with matching files.
 *
 * @param from Source directory to sync from.
 * @param to   Destination directory (cleared and replaced).
 */
inline void syncFiles(const std::filesystem::path& from, const std::filesystem::path& to) {
  std::filesystem::remove_all(to);
  std::filesystem::create_directories(to);
  if (std::filesystem::is_directory(from)) {
    for (const auto& dirent : std::filesystem::recursive_directory_iterator(from)) {
      if (dirent.is_regular_file() && isXmlFile(dirent.path())) {
        std::filesystem::copy(dirent.path(), to);
      }
    }
  }
}

}  // namespace sentinel::io

#endif  // INCLUDE_SENTINEL_UTIL_IO_HPP_
