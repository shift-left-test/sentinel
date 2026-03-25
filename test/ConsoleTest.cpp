/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "sentinel/Console.hpp"

namespace sentinel {

class ConsoleTest : public ::testing::Test {};

TEST_F(ConsoleTest, testOut) {
  testing::internal::CaptureStdout();
  Console::out("hello world");
  EXPECT_EQ("hello world\n", testing::internal::GetCapturedStdout());
}

TEST_F(ConsoleTest, testFormattedOut) {
  testing::internal::CaptureStdout();
  Console::out("{} {}", "hello", "world");
  EXPECT_EQ("hello world\n", testing::internal::GetCapturedStdout());
}

TEST_F(ConsoleTest, testErr) {
  testing::internal::CaptureStderr();
  Console::err("foo bar");
  EXPECT_EQ("foo bar\n", testing::internal::GetCapturedStderr());
}

TEST_F(ConsoleTest, testFormattedErr) {
  testing::internal::CaptureStderr();
  Console::err("{} {}", "foo", "bar");
  EXPECT_EQ("foo bar\n", testing::internal::GetCapturedStderr());
}

}  // namespace sentinel
