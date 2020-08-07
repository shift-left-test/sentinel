/*
  MIT License

  Copyright (c) 2020 Sangmo Kang

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

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <string>
#include "sentinel/DefaultLogger.hpp"


namespace sentinel {

class DefaultLoggerTest : public ::testing::Test {
 protected:
  static constexpr const char* LOGGER_NAME_MYLOGGER = "MYLOGGER";
  static constexpr const char* LOGGER_NAME_CUSTOMLOGGER = "CUSTOMLOGGER";
  static constexpr const char* LOG_MESSAGE_START = "START";
  static constexpr const char* LOG_MESSAGE_EMPTY = "";
  static constexpr const char* DEFAULT_LOG_FORMAT = "{name}::{type}::{message}";
  static constexpr const char* CUSTOM_LOG_FORMAT = "{message}({name}|{type})";
  static constexpr const char* CUSTOM_LOG_FORMAT_WRONG = "{wrong_arg}";
  static constexpr const char* CUSTOM_LOG_FORMAT_EMPTY = "";
  static constexpr const char* LOG_TYPE_INFO = "INFO";
  static constexpr const char* LOG_TYPE_ERROR = "ERROR";
  static constexpr const char* LOG_TYPE_WARN = "WARN";
};

TEST_F(DefaultLoggerTest, testDefaultLoggerInfoWithDefaultFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_MYLOGGER);

  testing::internal::CaptureStdout();
  logger.info(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStdout();

  EXPECT_EQ(
      fmt::format(DEFAULT_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_MYLOGGER),
          fmt::arg("type", LOG_TYPE_INFO),
          fmt::arg("message", LOG_MESSAGE_START)),
      capturedOutput);

  testing::internal::CaptureStdout();
  logger.info(LOG_MESSAGE_EMPTY);
  capturedOutput
      = testing::internal::GetCapturedStdout();

  EXPECT_EQ(
      fmt::format(DEFAULT_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_MYLOGGER),
          fmt::arg("type", LOG_TYPE_INFO),
          fmt::arg("message", LOG_MESSAGE_EMPTY)),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerInfoWithCustomFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT);

  testing::internal::CaptureStdout();
  logger.info(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStdout();

  EXPECT_EQ(
      fmt::format(CUSTOM_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_CUSTOMLOGGER),
          fmt::arg("type", LOG_TYPE_INFO),
          fmt::arg("message", LOG_MESSAGE_START)),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerInfoWithEmptyCustomFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT_EMPTY);

  testing::internal::CaptureStdout();
  logger.info(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStdout();

  EXPECT_EQ(std::string("\n"),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWarnWithDefaultFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_MYLOGGER);

  testing::internal::CaptureStderr();
  logger.warn(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(
      fmt::format(DEFAULT_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_MYLOGGER),
          fmt::arg("type", LOG_TYPE_WARN),
          fmt::arg("message", LOG_MESSAGE_START)),
      capturedOutput);

  testing::internal::CaptureStderr();
  logger.warn(LOG_MESSAGE_EMPTY);
  capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(
      fmt::format(DEFAULT_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_MYLOGGER),
          fmt::arg("type", LOG_TYPE_WARN),
          fmt::arg("message", LOG_MESSAGE_EMPTY)),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWarnWithCustomFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT);

  testing::internal::CaptureStderr();
  logger.warn(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(
      fmt::format(CUSTOM_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_CUSTOMLOGGER),
          fmt::arg("type", LOG_TYPE_WARN),
          fmt::arg("message", LOG_MESSAGE_START)),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWarnWithEmptyCustomFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT_EMPTY);

  testing::internal::CaptureStderr();
  logger.warn(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(std::string("\n"),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerErrWithDefaultFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_MYLOGGER);

  testing::internal::CaptureStderr();
  logger.error(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(
      fmt::format(DEFAULT_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_MYLOGGER),
          fmt::arg("type", LOG_TYPE_ERROR),
          fmt::arg("message", LOG_MESSAGE_START)),
      capturedOutput);

  testing::internal::CaptureStderr();
  logger.error(LOG_MESSAGE_EMPTY);
  capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(
      fmt::format(DEFAULT_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_MYLOGGER),
          fmt::arg("type", LOG_TYPE_ERROR),
          fmt::arg("message", LOG_MESSAGE_EMPTY)),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerErrWithCustomFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT);

  testing::internal::CaptureStderr();
  logger.error(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(
      fmt::format(CUSTOM_LOG_FORMAT + std::string("\n"),
          fmt::arg("name", LOGGER_NAME_CUSTOMLOGGER),
          fmt::arg("type", LOG_TYPE_ERROR),
          fmt::arg("message", LOG_MESSAGE_START)),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerErrWithEmptyCustomFormat) {
  sentinel::DefaultLogger logger(
      LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT_EMPTY);

  testing::internal::CaptureStderr();
  logger.error(LOG_MESSAGE_START);
  std::string capturedOutput
      = testing::internal::GetCapturedStderr();

  EXPECT_EQ(std::string("\n"),
      capturedOutput);
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWithWrongFormat) {
  EXPECT_THROW({
    try {
      sentinel::DefaultLogger logger(
          LOGGER_NAME_CUSTOMLOGGER, CUSTOM_LOG_FORMAT_WRONG);
    }
    catch (const sentinel::InvalidArgumentException& e){
      EXPECT_STREQ("argument not found", e.what());
      throw;
    }
  }, sentinel::InvalidArgumentException);
}

}  // namespace sentinel
