/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_LOGGER_HPP_
#define INCLUDE_SENTINEL_LOGGER_HPP_

#include <memory>
#include <string>


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
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    OFF = 5,
  };

  /**
   * @brief Return the logger instance
   *
   * @param name of the logger
   * @return logger instance
   */
  static std::shared_ptr<Logger> getLogger(const std::string& name);

  /**
   * @brief Return the logger instance
   *
   * @param name of the logger
   * @param format string
   * @return logger instance
   */
  static std::shared_ptr<Logger> getLogger(const std::string& name,
                                           const std::string& format);

  /**
   * @brief Set the logging level
   *
   * @param level of the logging
   */
  static void setDefaultLevel(Logger::Level level);

  /**
   * @brief Log a debug message
   *
   * @param message to log
   */
  void debug(const std::string& message);

  /**
   * @brief Log an information message
   *
   * @param message to log
   */
  void info(const std::string& message);

  /**
   * @brief Log a warning message
   *
   * @param message to log
   */
  void warn(const std::string& message);

  /**
   * @brief Log an error message
   *
   * @param message to log
   */
  void error(const std::string& message);

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
   * @param format string
   * @param level logging level
   */
  Logger(const std::string& name,
         const std::string& format,
         Logger::Level level);

  /**
   * @brief Return formatted string
   *
   * @param level of the logging
   * @param message to log
   * @return formatted string
   */
  std::string format(Logger::Level level, const std::string& message);

  /**
   * @brief Test if the level is allowed to log
   *
   * @param level of the logging
   * @return true if the level is allowed, false otherwise
   */
  bool isAllowed(Logger::Level level);

 private:
  std::string mName;
  std::string mFormat;
  Logger::Level mLevel;
};

}  // namespace sentinel


#endif  // INCLUDE_SENTINEL_LOGGER_HPP_
