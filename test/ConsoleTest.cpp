/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "sentinel/Console.hpp"

namespace sentinel {

class ConsoleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    capturedOut = CaptureHelper::getStdoutCapture();
    capturedErr = CaptureHelper::getStderrCapture();
  }

  void TearDown() override {
  }

  std::shared_ptr<CaptureHelper> capturedOut;
  std::shared_ptr<CaptureHelper> capturedErr;
};

TEST_F(ConsoleTest, testOut) {
  capturedOut->capture();
  Console::out("hello world");
  EXPECT_EQ("hello world\n", capturedOut->release());
}

TEST_F(ConsoleTest, testFormattedOut) {
  capturedOut->capture();
  Console::out("{} {}", "hello", "world");
  EXPECT_EQ("hello world\n", capturedOut->release());
}

TEST_F(ConsoleTest, testErr) {
  capturedErr->capture();
  Console::err("foo bar");
  EXPECT_EQ("foo bar\n", capturedErr->release());
}

TEST_F(ConsoleTest, testFormattedErr) {
  capturedErr->capture();
  Console::err("{} {}", "foo", "bar");
  EXPECT_EQ("foo bar\n", capturedErr->release());
}

}  // namespace sentinel
