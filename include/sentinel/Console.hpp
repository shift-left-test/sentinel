/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONSOLE_HPP_
#define INCLUDE_SENTINEL_CONSOLE_HPP_

#include <fmt/core.h>
#include <fmt/format.h>
#include <unistd.h>
#include <filesystem>  // NOLINT
#include <iostream>
#include <string>
#include <utility>

/// @cond INTERNAL
template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const std::filesystem::path& p, FormatContext& ctx) {  // NOLINT
    return fmt::formatter<std::string>::format(p.string(), ctx);
  }
};
/// @endcond

namespace sentinel::Console {

/**
 * @brief Print messages without a trailing newline.
 *
 * @param pattern to print
 * @param args arguments
 */
template <typename... Args>
inline void print(const std::string& pattern, Args&&... args) {
  std::cout << fmt::format(pattern, std::forward<Args>(args)...);
}

/**
 * @brief Print messages followed by a newline.
 *
 * @param pattern to print
 * @param args arguments
 */
template <typename... Args>
inline void out(const std::string& pattern, Args&&... args) {
  std::cout << fmt::format(pattern, std::forward<Args>(args)...) << '\n';
}

/**
 * @brief Flush the stdout buffer.
 */
inline void flush() {
  std::cout << std::flush;
}

/**
 * @brief Print error messages followed by a newline.
 *
 * @param pattern to print
 * @param args arguments
 */
template <typename... Args>
inline void err(const std::string& pattern, Args&&... args) {
  std::cerr << fmt::format(pattern, std::forward<Args>(args)...) << std::endl;
}

/**
 * @brief Prompts the user for confirmation.
 *
 * @param pattern to print
 * @param args arguments
 * @return true if the user confirms, false otherwise.
 */
template <typename... Args>
inline bool confirm(const std::string& pattern, Args&&... args) {
  std::cout << fmt::format(pattern, std::forward<Args>(args)...) << "\nProceed? [y/N] ";
  flush();

  if (isatty(STDIN_FILENO) == 0) {
    err("(non-interactive: defaulting to N)");
    out("Aborted.");
    return false;
  }

  std::string answer;
  std::getline(std::cin, answer);
  if (answer != "y" && answer != "Y") {
    out("Aborted.");
    return false;
  }
  return true;
}

}  // namespace sentinel::Console

#endif  // INCLUDE_SENTINEL_CONSOLE_HPP_
