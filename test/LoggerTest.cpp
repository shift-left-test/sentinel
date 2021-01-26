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
    off = Logger::getLogger("off");

    Logger::setDefaultLevel(Logger::Level::DEBUG);
    debug = Logger::getLogger("debug");

    Logger::setDefaultLevel(Logger::Level::ALL);
    all = Logger::getLogger("all");

    Logger::setDefaultLevel(Logger::Level::INFO);
    logger = Logger::getLogger("logger");
  }

  void TearDown() override {
    Logger::setDefaultLevel(Logger::Level::OFF);
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

  std::shared_ptr<Logger> off;
  std::shared_ptr<Logger> debug;
  std::shared_ptr<Logger> all;
  std::shared_ptr<Logger> logger;
};

TEST_F(LoggerTest, testInfoWithInitialLevel) {
  captureStdout();
  off->info("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testDebugWithInfoLevel) {
  captureStdout();
  logger->debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithInfoLevel) {
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("logger [INFO] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testWarnWithInfoLevel) {
  captureStderr();
  logger->warn("hello world");
  EXPECT_STREQ("logger [WARN] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testErrorWithInfoLevel) {
  captureStderr();
  logger->error("hello world");
  EXPECT_STREQ("logger [ERROR] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testDebugWithDebugLevel) {
  captureStdout();
  debug->debug("hello world");
  EXPECT_STREQ("debug [DEBUG] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogsWithAllLevel) {
  captureStdout();
  captureStderr();
  all->debug("D");
  all->info("I");
  all->warn("W");
  all->error("E");
  EXPECT_STREQ("all [DEBUG] D\nall [INFO] I\n",
               capturedStdout().c_str());
  EXPECT_STREQ("all [WARN] W\nall [ERROR] E\n",
               capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithOffLevelSet) {
  captureStdout();
  captureStderr();
  off->debug("D");
  off->info("I");
  off->warn("W");
  off->error("E");
  EXPECT_STREQ("", capturedStdout().c_str());
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogWithDifferentFormat) {
  logger = Logger::getLogger("another", "{name}:{level}:{message}");
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("another:INFO:hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogWithEmptyFormat) {
  logger = Logger::getLogger("empty", "");
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogWithWrongFormat) {
  EXPECT_THROW(Logger::getLogger("wrong", "{wrong}"),
               InvalidArgumentException);
}

TEST_F(LoggerTest, testLoggersAreShared) {
  auto logger1 = Logger::getLogger("shared", "{name}:{level}:{message}");
  auto logger2 = Logger::getLogger("shared");
  captureStdout();
  logger1->info("1");
  logger2->info("2");
  EXPECT_STREQ("shared:INFO:1\nshared:INFO:2\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testSetLevelChangesGlobalLogLevel) {
  captureStdout();
  logger->debug("1");
  EXPECT_STREQ("", capturedStdout().c_str());

  Logger::setDefaultLevel(Logger::Level::DEBUG);
  logger = Logger::getLogger("another2");

  captureStdout();
  logger->debug("1");
  EXPECT_STREQ("another2 [DEBUG] 1\n", capturedStdout().c_str());
}

}  // namespace sentinel
