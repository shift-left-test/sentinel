/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
  void setLevel(Logger::Level level);

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

 private:
  /**
   * @brief Default constructor
   *
   * @param name of the logger
   * @param format string
   */
  Logger(const std::string& name, const std::string& format);

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
