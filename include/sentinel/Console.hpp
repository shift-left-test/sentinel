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

namespace sentinel {
namespace Console {

/**
 * @brief Print messages.
 *
 * @param pattern to print
 * @param args arguments
 */
template <typename... Args>
inline void out(const std::string& pattern, Args&&... args) {
  std::cout << fmt::format(pattern, std::forward<Args>(args)...) << std::endl;
}

/**
 * @brief Print messages.
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
    std::cout << fmt::format(pattern, std::forward<Args>(args)...) << " [y/N] ";
    std::cout.flush();

    std::string answer;
    std::getline(std::cin, answer);

    if (answer == "y" || answer == "Y") {
      return true;
    } else if (answer == "n" || answer == "N") {
      return false;
    } else if (answer.empty()) {
      return false;
    }
  }
}

}  // namespace Console
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CONSOLE_HPP_
