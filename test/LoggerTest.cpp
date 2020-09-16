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

#include <gtest/gtest.h>
#include <memory>
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"


namespace sentinel {

class LoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    logger = Logger::getLogger("logger");
  }

  void captureStdout() {
    testing::internal::CaptureStdout();
  }

  std::string capturedStdout() {
    return testing::internal::GetCapturedStdout();
  }

  void captureStderr() {
    testing::internal::CaptureStderr();
  }

  std::string capturedStderr() {
    return testing::internal::GetCapturedStderr();
  }

  std::shared_ptr<Logger> logger;
};

TEST_F(LoggerTest, testDebugWithDefaultState) {
  captureStdout();
  logger->debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithDefaultState) {
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("logger:INFO: hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testWarnWithDefaultState) {
  captureStderr();
  logger->warn("hello world");
  EXPECT_STREQ("logger:WARN: hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testErrorWithDefaultState) {
  captureStderr();
  logger->error("hello world");
  EXPECT_STREQ("logger:ERROR: hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testDebugWhenDebugLevelSet) {
  captureStdout();
  logger->setLevel(Logger::Level::DEBUG);
  logger->debug("hello world");
  EXPECT_STREQ("logger:DEBUG: hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogsWithOnLevelSet) {
  captureStdout();
  captureStderr();
  logger->setLevel(Logger::Level::ALL);
  logger->debug("D");
  logger->info("I");
  logger->warn("W");
  logger->error("E");
  EXPECT_STREQ("logger:DEBUG: D\nlogger:INFO: I\n",
               capturedStdout().c_str());
  EXPECT_STREQ("logger:WARN: W\nlogger:ERROR: E\n",
               capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithOffLevelSet) {
  captureStdout();
  captureStderr();
  logger->setLevel(Logger::Level::OFF);
  logger->debug("D");
  logger->info("I");
  logger->warn("W");
  logger->error("E");
  EXPECT_STREQ("", capturedStdout().c_str());
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogWithDifferentFormat) {
  logger = Logger::getLogger("logger", "{name} [{level}] {message}");
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("logger [INFO] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogWithEmptyFormat) {
  logger = Logger::getLogger("logger", "");
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogWithWrongFormat) {
  EXPECT_THROW(Logger::getLogger("logger", "{wrong}"),
               InvalidArgumentException);
}

}  // namespace sentinel
