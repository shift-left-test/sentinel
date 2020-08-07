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
#include <memory>
#include "sentinel/Logger.hpp"
#include "sentinel/DefaultLogger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"


namespace sentinel {

class DefaultLoggerTest : public ::testing::Test {
 protected:
  void captureStdout() {
    testing::internal::CaptureStdout();
  }

  std::string getCapturedStdout() {
    return testing::internal::GetCapturedStdout();
  }

  void captureStderr() {
    testing::internal::CaptureStderr();
  }

  std::string getCapturedStderr() {
    return testing::internal::GetCapturedStderr();
  }

  void CHECK_INFO(const std::string& message,
                  const std::string& expected) {
    testing::internal::CaptureStdout();
    logger->info(message);
    EXPECT_EQ(expected, testing::internal::GetCapturedStdout());
  }

  void CHECK_WARN(const std::string& message,
                  const std::string& expected) {
    testing::internal::CaptureStderr();
    logger->warn(message);
    EXPECT_EQ(expected, testing::internal::GetCapturedStderr());
  }

  void CHECK_ERROR(const std::string& message,
                   const std::string& expected) {
    testing::internal::CaptureStderr();
    logger->error(message);
    EXPECT_EQ(expected, testing::internal::GetCapturedStderr());
  }

  static constexpr const char* LOGGER_NAME = "MYLOGGER";
  static constexpr const char* START_MESSAGE = "START";
  static constexpr const char* EMPTY_MESSAGE = "";
  static constexpr const char* DEFAULT_FORMAT = "{name}::{type}::{message}";
  static constexpr const char* CUSTOM_FORMAT = "{message}({name}|{type})";
  static constexpr const char* WRONG_FORMAT = "{wrong_arg}";
  static constexpr const char* EMPTY_FORMAT = "";

  std::shared_ptr<Logger> logger;
};

TEST_F(DefaultLoggerTest, testDefaultLoggerInfoWithDefaultFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME);
  CHECK_INFO(START_MESSAGE, "MYLOGGER::INFO::START\n");
  CHECK_INFO(EMPTY_MESSAGE, "MYLOGGER::INFO::\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerInfoWithCustomFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME, CUSTOM_FORMAT);
  CHECK_INFO(START_MESSAGE, "START(MYLOGGER|INFO)\n");
  CHECK_INFO(EMPTY_MESSAGE, "(MYLOGGER|INFO)\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerInfoWithEmptyCustomFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME, EMPTY_FORMAT);
  CHECK_INFO(START_MESSAGE, "\n");
  CHECK_INFO(EMPTY_MESSAGE, "\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWarnWithDefaultFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME);
  CHECK_WARN(START_MESSAGE, "MYLOGGER::WARN::START\n");
  CHECK_WARN(EMPTY_MESSAGE, "MYLOGGER::WARN::\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWarnWithCustomFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME, CUSTOM_FORMAT);
  CHECK_WARN(START_MESSAGE, "START(MYLOGGER|WARN)\n");
  CHECK_WARN(EMPTY_MESSAGE, "(MYLOGGER|WARN)\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWarnWithEmptyCustomFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME, EMPTY_FORMAT);
  CHECK_WARN(START_MESSAGE, "\n");
  CHECK_WARN(EMPTY_MESSAGE, "\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerErrWithDefaultFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME);
  CHECK_ERROR(START_MESSAGE, "MYLOGGER::ERROR::START\n");
  CHECK_ERROR(EMPTY_MESSAGE, "MYLOGGER::ERROR::\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerErrWithCustomFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME, CUSTOM_FORMAT);
  CHECK_ERROR(START_MESSAGE, "START(MYLOGGER|ERROR)\n");
  CHECK_ERROR(EMPTY_MESSAGE, "(MYLOGGER|ERROR)\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerErrWithEmptyCustomFormat) {
  logger = std::make_shared<DefaultLogger>(LOGGER_NAME, EMPTY_FORMAT);
  CHECK_ERROR(START_MESSAGE, "\n");
  CHECK_ERROR(EMPTY_MESSAGE, "\n");
}

TEST_F(DefaultLoggerTest, testDefaultLoggerWithWrongFormat) {
  EXPECT_THROW({
    try {
      DefaultLogger logger(LOGGER_NAME, WRONG_FORMAT);
    }
    catch (const InvalidArgumentException& e){
      EXPECT_STREQ("argument not found", e.what());
      throw;
    }
  }, InvalidArgumentException);
}

}  // namespace sentinel
