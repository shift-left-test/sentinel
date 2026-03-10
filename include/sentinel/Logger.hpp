/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_LOGGER_HPP_
#define INCLUDE_SENTINEL_LOGGER_HPP_

#include <fmt/core.h>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

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
    DEBUG = 1,
    VERBOSE = 2,
    INFO = 3,
    WARN = 4,
    ERROR = 5,
    OFF = 6,
  };

  /**
   * @brief Return the logger instance
   *
   * @param name of the logger
   * @return logger instance
   */
  static std::shared_ptr<Logger> getLogger(const std::string& name);

  /**
   * @brief Set the logging level
   *
   * @param level of the logging
   */
  static void setDefaultLevel(Logger::Level level);

  /**
   * @brief Clear the logger cache.
   *        Primarily for use in tests to ensure isolation between test cases.
   */
  static void clearCache();

  /**
   * @brief Log a debug message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  void debug(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::DEBUG)) {
      std::cout << format("debug", pattern, std::forward<Args>(args)...) << std::endl;
    }
  }

  /**
   * @brief Log a verbose message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  void verbose(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::VERBOSE)) {
      std::cout << format("verbose", pattern, std::forward<Args>(args)...) << std::endl;
    }
  }

  /**
   * @brief Log an information message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  void info(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::INFO)) {
      std::cout << format("info", pattern, std::forward<Args>(args)...) << std::endl;
    }
  }

  /**
   * @brief Log a warning message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  void warn(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::WARN)) {
      std::cerr << format("warn", pattern, std::forward<Args>(args)...) << std::endl;
    }
  }

  /**
   * @brief Log an error message
   *
   * @param pattern to log
   * @param args arguments
   */
  template <typename... Args>
  void error(const std::string& pattern, Args&&... args) {
    if (isAllowed(Level::ERROR)) {
      std::cerr << format("error", pattern, std::forward<Args>(args)...) << std::endl;
    }
  }

  /**
   * @brief update Logger's level using defaultLevel
   *
   * @param level of the Logger
   */
  void setLevel(Logger::Level level);

 private:
  /**
   * @brief Default constructor
   *
   * @param name of the logger
   * @param level logging level
   */
  Logger(const std::string& name, Logger::Level level);

  /**
   * @brief Return formatted string
   *
   * @param level of the logging
   * @param pattern to log
   * @param args arguments
   * @return formatted string
   */
  template <typename... Args>
  std::string format(const std::string& level, const std::string& pattern, Args&&... args) {
    return fmt::format("[{}] ", level) + fmt::format(pattern, std::forward<Args>(args)...);
  }

  /**
   * @brief Test if the level is allowed to log
   *
   * @param level of the logging
   * @return true if the level is allowed, false otherwise
   */
  bool isAllowed(Logger::Level level);

 private:
  std::string mName;
  Logger::Level mLevel;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_LOGGER_HPP_
