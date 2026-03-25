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
    mStdoutCapture = CaptureHelper::getStdoutCapture();
    mStderrCapture = CaptureHelper::getStderrCapture();
  }

  void TearDown() override {
    Logger::setLevel(Logger::Level::OFF);
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

 private:
  std::shared_ptr<CaptureHelper> mStdoutCapture;
  std::shared_ptr<CaptureHelper> mStderrCapture;
};

TEST_F(LoggerTest, testInfoWithInitialLevel) {
  Logger::setLevel(Logger::Level::OFF);
  captureStderr();
  Logger::info("hello world");
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testVerboseWithInfoLevel) {
  captureStderr();
  Logger::verbose("hello world");
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testInfoWithInfoLevel) {
  captureStderr();
  Logger::info("hello world");
  EXPECT_STREQ("[INFO]  hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testWarnWithInfoLevel) {
  captureStderr();
  Logger::warn("hello world");
  EXPECT_STREQ("[WARN]  hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testErrorWithInfoLevel) {
  captureStderr();
  Logger::error("hello world");
  EXPECT_STREQ("[ERROR] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testVerboseWithDebugLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStderr();
  Logger::verbose("hello world");
  EXPECT_STREQ("[VERBOSE] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithAllLevel) {
  Logger::setLevel(Logger::Level::ALL);
  captureStdout();
  captureStderr();
  Logger::verbose("D");
  Logger::info("I");
  Logger::warn("W");
  Logger::error("E");
  EXPECT_STREQ("", capturedStdout().c_str());
  EXPECT_STREQ("[VERBOSE] D\n[INFO]  I\n[WARN]  W\n[ERROR] E\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithOffLevelSet) {
  Logger::setLevel(Logger::Level::OFF);
  captureStdout();
  captureStderr();
  Logger::verbose("D");
  Logger::info("I");
  Logger::warn("W");
  Logger::error("E");
  EXPECT_STREQ("", capturedStdout().c_str());
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testSetLevelAffectsAllLogs) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStderr();
  Logger::verbose("1");
  Logger::verbose("2");
  EXPECT_STREQ("[VERBOSE] 1\n[VERBOSE] 2\n", capturedStderr().c_str());
}

}  // namespace sentinel
