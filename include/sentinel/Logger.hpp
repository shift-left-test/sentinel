/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_LOGGER_HPP_
#define INCLUDE_SENTINEL_LOGGER_HPP_

#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <utility>
#include "sentinel/Console.hpp"

namespace sentinel {

/**
 * @brief Logger class
 */
class Logger {
 public:
  /**
   * @brief Logger level enumeration
   */
  enum class Level : int {
    ALL = 0,
    VERBOSE = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    OFF = 5,
  };

  Logger() = delete;

  /**
   * @brief Set the global logging level.
   *
   * @param level of the logging
   */
  static void setLevel(Logger::Level level);

  /**
   * @brief Log a verbose message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  static void verbose(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::VERBOSE)) {
      Console::err("[VERBOSE] {}", fmt::format(pattern, std::forward<Args>(args)...));
    }
  }

  /**
   * @brief Log an information message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  static void info(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::INFO)) {
      Console::err("[INFO]  {}", fmt::format(pattern, std::forward<Args>(args)...));
    }
  }

  /**
   * @brief Log a warning message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  static void warn(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::WARN)) {
      Console::err("[WARN]  {}", fmt::format(pattern, std::forward<Args>(args)...));
    }
  }

  /**
   * @brief Log an error message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  static void error(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::ERROR)) {
      Console::err("[ERROR] {}", fmt::format(pattern, std::forward<Args>(args)...));
    }
  }

 private:
  /**
   * @brief Test if the level is allowed to log
   *
   * @param level of the logging
   * @return true if the level is allowed, false otherwise
   */
  static bool isAllowed(Logger::Level level);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_LOGGER_HPP_
