/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONSOLE_HPP_
#define INCLUDE_SENTINEL_CONSOLE_HPP_

#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <string>

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
  print("{}\n", fmt::format(pattern, std::forward<Args>(args)...));
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
  while (true) {
    print("{} [y/N] " , fmt::format(pattern, std::forward<Args>(args)...));
    flush();

    std::string answer;
    std::getline(std::cin, answer);

    if (answer == "y" || answer == "Y") {
      return true;
    } else if (answer == "n" || answer == "N") {
      return false;
    } else if (answer.empty()) {
      return false;
    }
    err("Error: '{}' is not a valid input. Please enter 'y' or 'n'.\n", answer);
  }
}

}  // namespace sentinel::Console

#endif  // INCLUDE_SENTINEL_CONSOLE_HPP_
