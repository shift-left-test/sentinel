/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONSOLE_HPP_
#define INCLUDE_SENTINEL_CONSOLE_HPP_

#include <fmt/core.h>
#include <iostream>
#include <string>
#include <utility>
#include "sentinel/util/formatter.hpp"

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
  std::cout << fmt::format(pattern, std::forward<Args>(args)...) << '\n' << std::flush;
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

}  // namespace sentinel::Console

#endif  // INCLUDE_SENTINEL_CONSOLE_HPP_
