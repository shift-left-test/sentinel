/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_LOGGER_HPP_
#define INCLUDE_SENTINEL_LOGGER_HPP_

#include <fmt/core.h>
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
      logMessage(kColorDarkGray, fmt::format(pattern, std::forward<Args>(args)...));
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
      logMessage(kColorLightGray, fmt::format("INFO: {}", fmt::format(pattern, std::forward<Args>(args)...)));
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
      logMessage(kColorYellow, fmt::format("WARN: {}", fmt::format(pattern, std::forward<Args>(args)...)));
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
      logMessage(kColorRed, fmt::format("ERROR: {}", fmt::format(pattern, std::forward<Args>(args)...)));
    }
  }

 private:
  static constexpr const char* kColorReset = "\033[0m";
  static constexpr const char* kColorDarkGray = "\033[90m";
  static constexpr const char* kColorLightGray = "\033[37m";
  static constexpr const char* kColorYellow = "\033[33m";
  static constexpr const char* kColorRed = "\033[31m";

  /**
   * @brief Test if the level is allowed to log
   *
   * @param level of the logging
   * @return true if the level is allowed, false otherwise
   */
  static bool isAllowed(Logger::Level level);

  /**
   * @brief Test if stderr is a TTY
   *
   * @return true if stderr is connected to a terminal, false otherwise
   */
  static bool isTty();

  /**
   * @brief Log a message with the given color, stripping color codes if not a TTY
   *
   * @param color ANSI color escape sequence
   * @param msg message to log
   */
  static void logMessage(const char* color, const std::string& msg);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_LOGGER_HPP_
