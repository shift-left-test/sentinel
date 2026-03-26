/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_IO_HPP_
#define INCLUDE_SENTINEL_UTIL_IO_HPP_

#include <algorithm>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/formatter.hpp"

namespace sentinel::io {

/**
 * @brief Ensure the given path is a directory, creating it if it does not exist.
 *
 * @param dirPath path to ensure as a directory
 * @throw InvalidArgumentException if the path exists but is not a directory
 */
inline void ensureDirectoryExists(const std::filesystem::path& dirPath) {
  if (std::filesystem::exists(dirPath)) {
    if (!std::filesystem::is_directory(dirPath)) {
      throw InvalidArgumentException(fmt::format("'{}' is not a directory", dirPath));
    }
  } else {
    std::filesystem::create_directories(dirPath);
  }
}

/**
 * @brief Synchronize @p to with files matching @p exts from @p from.
 *        Replaces the entire contents of @p to with the matching files.
 *
 * @param from Source directory to sync from.
 * @param to   Destination directory (cleared and replaced).
 * @param exts File extensions to include (empty = all files).
 */
inline void syncFiles(const std::filesystem::path& from, const std::filesystem::path& to,
                      const std::vector<std::string>& exts) {
  std::filesystem::remove_all(to);
  std::filesystem::create_directories(to);
  if (std::filesystem::is_directory(from)) {
    for (const auto& dirent : std::filesystem::recursive_directory_iterator(from)) {
      if (dirent.is_regular_file()) {
        std::string ext = dirent.path().extension().string();
        if (ext.size() > 1) ext = ext.substr(1);
        if (exts.empty() || std::find(exts.begin(), exts.end(), ext) != exts.end()) {
          std::filesystem::copy(dirent.path(), to);
        }
      }
    }
  }
}

/**
 * @brief Synchronize @p to with all files from @p from.
 *        Replaces the entire contents of @p to with the source files.
 *
 * @param from Source directory to sync from.
 * @param to   Destination directory (cleared and replaced).
 */
inline void syncFiles(const std::filesystem::path& from, const std::filesystem::path& to) {
  syncFiles(from, to, {});
}

}  // namespace sentinel::io

#endif  // INCLUDE_SENTINEL_UTIL_IO_HPP_
