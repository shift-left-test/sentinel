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
  captureStdout();
  Logger::info("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testDebugWithInfoLevel) {
  captureStdout();
  Logger::debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testVerboseWithInfoLevel) {
  captureStdout();
  Logger::verbose("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testDebugWithVerboseLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStdout();
  Logger::debug("hello world");
  EXPECT_STREQ("", capturedStdout().c_str());
}

TEST_F(LoggerTest, testVerboseWithVerboseLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStdout();
  Logger::verbose("hello world");
  EXPECT_STREQ("[verbose] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithVerboseLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  captureStdout();
  Logger::info("hello world");
  EXPECT_STREQ("[info] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testInfoWithInfoLevel) {
  captureStdout();
  Logger::info("hello world");
  EXPECT_STREQ("[info] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testWarnWithInfoLevel) {
  captureStderr();
  Logger::warn("hello world");
  EXPECT_STREQ("[warn] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testErrorWithInfoLevel) {
  captureStderr();
  Logger::error("hello world");
  EXPECT_STREQ("[error] hello world\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testDebugWithDebugLevel) {
  Logger::setLevel(Logger::Level::DEBUG);
  captureStdout();
  Logger::debug("hello world");
  EXPECT_STREQ("[debug] hello world\n", capturedStdout().c_str());
}

TEST_F(LoggerTest, testLogsWithAllLevel) {
  Logger::setLevel(Logger::Level::ALL);
  captureStdout();
  captureStderr();
  Logger::debug("D");
  Logger::verbose("V");
  Logger::info("I");
  Logger::warn("W");
  Logger::error("E");
  EXPECT_STREQ("[debug] D\n[verbose] V\n[info] I\n", capturedStdout().c_str());
  EXPECT_STREQ("[warn] W\n[error] E\n", capturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithOffLevelSet) {
  Logger::setLevel(Logger::Level::OFF);
  captureStdout();
  captureStderr();
  Logger::debug("D");
  Logger::info("I");
  Logger::warn("W");
  Logger::error("E");
  EXPECT_STREQ("", capturedStdout().c_str());
  EXPECT_STREQ("", capturedStderr().c_str());
}

TEST_F(LoggerTest, testSetLevelAffectsAllLogs) {
  Logger::setLevel(Logger::Level::DEBUG);
  captureStdout();
  Logger::debug("1");
  Logger::debug("2");
  EXPECT_STREQ("[debug] 1\n[debug] 2\n", capturedStdout().c_str());
}

}  // namespace sentinel
