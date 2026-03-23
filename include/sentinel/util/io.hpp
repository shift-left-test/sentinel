/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_IO_HPP_
#define INCLUDE_SENTINEL_UTIL_IO_HPP_

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include "sentinel/exceptions/InvalidArgumentException.hpp"

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
      throw InvalidArgumentException(fmt::format("'{}' is not a directory", dirPath.string()));
    }
  } else {
    std::filesystem::create_directories(dirPath);
  }
}

}  // namespace sentinel::io

#endif  // INCLUDE_SENTINEL_UTIL_IO_HPP_
