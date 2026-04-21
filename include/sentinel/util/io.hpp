/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_IO_HPP_
#define INCLUDE_SENTINEL_UTIL_IO_HPP_

#include <cstddef>
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
 * @throw InvalidArgumentException if directory creation fails
 */
inline void ensureDirectoryExists(const std::filesystem::path& dirPath) {
  std::error_code ec;
  std::filesystem::create_directories(dirPath, ec);
  if (ec) {
    throw InvalidArgumentException(fmt::format(
        "Failed to create directory '{}': {}", dirPath, ec.message()));
  }
}

/**
 * @brief Synchronize @p to with XML files from @p from, preserving relative paths.
 *        Replaces the entire contents of @p to with matching files, mirroring
 *        the source subdirectory layout so that files with the same name in
 *        different subdirectories do not collide.
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
        const std::filesystem::path rel = std::filesystem::relative(dirent.path(), from);
        const std::filesystem::path dest = to / rel;
        std::filesystem::create_directories(dest.parent_path());
        std::filesystem::copy(dirent.path(), dest);
      }
    }
  }
}

/**
 * @brief Read the last N lines from a file.
 *
 * @param path File to read.
 * @param n Maximum number of lines to return.
 * @return Last n lines joined by newline, or empty string if file is empty or unreadable.
 */
std::string readLastLines(const std::filesystem::path& path, std::size_t n);

/**
 * @brief Append the last N lines of a log file to a message string.
 *
 * Lines are wrapped with separator headers and indented for visual clarity.
 * If the file is empty or unreadable, the message is left unchanged.
 *
 * @param msg Message to append to (modified in place).
 * @param logPath Log file to read from.
 * @param n Maximum number of lines to append.
 * @param label Label for the separator header (default: "output").
 */
void appendLogTail(std::string* msg, const std::filesystem::path& logPath,
                   std::size_t n, const std::string& label = "output");

}  // namespace sentinel::io

#endif  // INCLUDE_SENTINEL_UTIL_IO_HPP_
