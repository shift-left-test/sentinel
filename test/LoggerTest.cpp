/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

class LoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Logger::setLevel(Logger::Level::INFO);
  }

  void TearDown() override {
    Logger::setLevel(Logger::Level::OFF);
  }
};

TEST_F(LoggerTest, testInfoWithInitialLevel) {
  Logger::setLevel(Logger::Level::OFF);
  testing::internal::CaptureStderr();
  Logger::info("hello world");
  EXPECT_STREQ("", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testVerboseWithInfoLevel) {
  testing::internal::CaptureStderr();
  Logger::verbose("hello world");
  EXPECT_STREQ("", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testInfoWithInfoLevel) {
  testing::internal::CaptureStderr();
  Logger::info("hello world");
  EXPECT_STREQ("[INFO]  hello world\n", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testWarnWithInfoLevel) {
  testing::internal::CaptureStderr();
  Logger::warn("hello world");
  EXPECT_STREQ("[WARN]  hello world\n", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testErrorWithInfoLevel) {
  testing::internal::CaptureStderr();
  Logger::error("hello world");
  EXPECT_STREQ("[ERROR] hello world\n", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testVerboseWithDebugLevel) {
  Logger::setLevel(Logger::Level::VERBOSE);
  testing::internal::CaptureStderr();
  Logger::verbose("hello world");
  EXPECT_STREQ("hello world\n", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithAllLevel) {
  Logger::setLevel(Logger::Level::ALL);
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  Logger::verbose("D");
  Logger::info("I");
  Logger::warn("W");
  Logger::error("E");
  EXPECT_STREQ("", testing::internal::GetCapturedStdout().c_str());
  EXPECT_STREQ("D\n[INFO]  I\n[WARN]  W\n[ERROR] E\n",
               testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testLogsWithOffLevelSet) {
  Logger::setLevel(Logger::Level::OFF);
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  Logger::verbose("D");
  Logger::info("I");
  Logger::warn("W");
  Logger::error("E");
  EXPECT_STREQ("", testing::internal::GetCapturedStdout().c_str());
  EXPECT_STREQ("", testing::internal::GetCapturedStderr().c_str());
}

TEST_F(LoggerTest, testSetLevelAffectsAllLogs) {
  Logger::setLevel(Logger::Level::VERBOSE);
  testing::internal::CaptureStderr();
  Logger::verbose("1");
  Logger::verbose("2");
  EXPECT_STREQ("1\n2\n", testing::internal::GetCapturedStderr().c_str());
}

}  // namespace sentinel
