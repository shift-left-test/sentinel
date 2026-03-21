/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

class LoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Logger::setLevel(Logger::Level::INFO);
    logger = Logger::getLogger("logger");
    mStdoutCapture = CaptureHelper::getStdoutCapture();
    mStderrCapture = CaptureHelper::getStderrCapture();
  }

  void TearDown() override {
    Logger::setLevel(Logger::Level::OFF);
    Logger::clearCache();
  }

  void captureStdout() {
    mStdoutCapture->capture();
  }

  std::string capturedStdout() {
    return mStdoutCapture->release();
  }

  void captureStderr() {
    mStderrCapture->capture();
  }

  std::string capturedStderr() {
    return mStderrCapture->release();
  }

  std::shared_ptr<Logger> logger;

 private:
  std::shared_ptr<CaptureHelper> mStdoutCapture;
  std::shared_ptr<CaptureHelper> mStderrCapture;
};

TEST_F(LoggerTest, testInfoWithInitialLevel) {
  Logger::setLevel(Logger::Level::OFF);
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testDebugWithInfoLevel) {
  captureStdout();
  logger->debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testVerboseWithInfoLevel) {
  captureStdout();
  logger->verbose("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testDebugWithVerboseLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStdout();
  logger->debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testVerboseWithVerboseLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStdout();
  logger->verbose("hello world");
  EXPECT_STREQ("[verbose] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithVerboseLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("[info] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithInfoLevel) {
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("[info] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testWarnWithInfoLevel) {
  captureStderr();
  logger->warn("hello world");
  EXPECT_STREQ("[warn] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testErrorWithInfoLevel) {
  captureStderr();
  logger->error("hello world");
  EXPECT_STREQ("[error] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testDebugWithDebugLevel) {
  Logger::setLevel(Logger::Level::DEBUG);
  captureStdout();
  logger->debug("hello world");
  EXPECT_STREQ("[debug] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogsWithAllLevel) {
  Logger::setLevel(Logger::Level::ALL);
  captureStdout();
  captureStderr();
  logger->debug("D");
  logger->verbose("V");
  logger->info("I");
  logger->warn("W");
  logger->error("E");
  EXPECT_STREQ("[debug] D\n[verbose] V\n[info] I\n", capturedStdout().c_str());
  EXPECT_STREQ("[warn] W\n[error] E\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithOffLevelSet) {
  Logger::setLevel(Logger::Level::OFF);
  captureStdout();
  captureStderr();
  logger->debug("D");
  logger->info("I");
  logger->warn("W");
  logger->error("E");
  EXPECT_STREQ("", capturedStdout().c_str());
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLoggersAreShared) {
  auto logger1 = Logger::getLogger("shared");
  auto logger2 = Logger::getLogger("shared");
  EXPECT_EQ(logger1.get(), logger2.get());
}

TEST_F(LoggerTest, testSetLevelAffectsExistingLoggers) {
  auto logger1 = Logger::getLogger("a");
  auto logger2 = Logger::getLogger("b");

  Logger::setLevel(Logger::Level::DEBUG);
  captureStdout();
  logger1->debug("1");
  logger2->debug("2");
  EXPECT_STREQ("[debug] 1\n[debug] 2\n", capturedStdout().c_str());
}

}  // namespace sentinel
