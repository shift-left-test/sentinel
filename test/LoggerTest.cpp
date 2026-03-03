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
    Logger::setDefaultLevel(Logger::Level::OFF);
    off = Logger::getLogger("off");
    off->setLevel(Logger::Level::OFF);

    Logger::setDefaultLevel(Logger::Level::DEBUG);
    debug = Logger::getLogger("debug");

    Logger::setDefaultLevel(Logger::Level::ALL);
    all = Logger::getLogger("all");

    Logger::setDefaultLevel(Logger::Level::VERBOSE);
    verboseLogger = Logger::getLogger("verboseLogger");

    Logger::setDefaultLevel(Logger::Level::INFO);
    logger = Logger::getLogger("logger");

    mStdoutCapture = CaptureHelper::getStdoutCapture();
    mStderrCapture = CaptureHelper::getStderrCapture();
  }

  void TearDown() override {
    Logger::setDefaultLevel(Logger::Level::OFF);
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

  std::shared_ptr<Logger> off;
  std::shared_ptr<Logger> debug;
  std::shared_ptr<Logger> all;
  std::shared_ptr<Logger> verboseLogger;
  std::shared_ptr<Logger> logger;

 private:
  std::shared_ptr<CaptureHelper> mStdoutCapture;
  std::shared_ptr<CaptureHelper> mStderrCapture;
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

TEST_F(LoggerTest, testVerboseWithInfoLevel) {
  captureStdout();
  logger->verbose("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testDebugWithVerboseLevel) {
  captureStdout();
  verboseLogger->debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testVerboseWithVerboseLevel) {
  captureStdout();
  verboseLogger->verbose("hello world");
  EXPECT_STREQ("[verbose] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithVerboseLevel) {
  captureStdout();
  verboseLogger->info("hello world");
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
  captureStdout();
  debug->debug("hello world");
  EXPECT_STREQ("[debug] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogsWithAllLevel) {
  captureStdout();
  captureStderr();
  all->debug("D");
  all->verbose("V");
  all->info("I");
  all->warn("W");
  all->error("E");
  EXPECT_STREQ("[debug] D\n[verbose] V\n[info] I\n", capturedStdout().c_str());
  EXPECT_STREQ("[warn] W\n[error] E\n", capturedStderr().c_str());
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
  EXPECT_STREQ("another:info:hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogWithEmptyFormat) {
  logger = Logger::getLogger("empty", "");
  captureStdout();
  logger->info("hello world");
  EXPECT_STREQ("\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogWithWrongFormat) {
  EXPECT_THROW(Logger::getLogger("wrong", "{wrong}"), InvalidArgumentException);
}

TEST_F(LoggerTest, testLoggersAreShared) {
  auto logger1 = Logger::getLogger("shared", "{name}:{level}:{message}");
  auto logger2 = Logger::getLogger("shared");
  captureStdout();
  logger1->info("1");
  logger2->info("2");
  EXPECT_STREQ("shared:info:1\nshared:info:2\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testSetLevelChangesGlobalLogLevel) {
  captureStdout();
  logger->debug("1");
  EXPECT_STREQ("", capturedStdout().c_str());

  Logger::setDefaultLevel(Logger::Level::DEBUG);
  logger = Logger::getLogger("another2");

  captureStdout();
  logger->debug("1");
  EXPECT_STREQ("[debug] 1\n", capturedStdout().c_str());
}

}  // namespace sentinel
