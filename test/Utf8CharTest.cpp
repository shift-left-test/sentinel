/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "sentinel/util/Utf8Char.hpp"

namespace sentinel {

class Utf8CharTest : public ::testing::Test {};

TEST_F(Utf8CharTest, CStrReturnsUnderlyingPointer) {
  Utf8Char ch("\xe2\x9c\x93");
  EXPECT_STREQ(ch.c_str(), "\xe2\x9c\x93");
}

TEST_F(Utf8CharTest, OperatorMultiplyRepeatsCharacter) {
  Utf8Char ch("\xe2\x94\x81");
  std::string result = ch * 3;
  EXPECT_EQ(result, "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81");
}

TEST_F(Utf8CharTest, OperatorMultiplyZeroReturnsEmpty) {
  Utf8Char ch("\xe2\x94\x81");
  std::string result = ch * 0;
  EXPECT_EQ(result, "");
}

TEST_F(Utf8CharTest, OperatorMultiplyOneReturnsSingleChar) {
  Utf8Char ch("\xe2\x9c\x93");
  std::string result = ch * 1;
  EXPECT_EQ(result, "\xe2\x9c\x93");
}

TEST_F(Utf8CharTest, FmtFormatSupport) {
  Utf8Char ch("\xe2\x9c\x93");
  std::string result = fmt::format("icon: {}", ch);
  EXPECT_EQ(result, "icon: \xe2\x9c\x93");
}

TEST_F(Utf8CharTest, ConstantsAreDefined) {
  EXPECT_STREQ(Utf8Char::CheckMark.c_str(), "\xe2\x9c\x93");
  EXPECT_STREQ(Utf8Char::CrossMark.c_str(), "\xe2\x9c\x97");
  EXPECT_STREQ(Utf8Char::Warning.c_str(), "\xe2\x9a\xa0");
  EXPECT_STREQ(Utf8Char::ArrowRight.c_str(), "\xe2\x86\x92");
  EXPECT_STREQ(Utf8Char::ArrowLeft.c_str(), "\xe2\x86\x90");
  EXPECT_STREQ(Utf8Char::ArrowHook.c_str(), "\xe2\x86\xaa");
  EXPECT_STREQ(Utf8Char::EmDash.c_str(), "\xe2\x80\x94");
  EXPECT_STREQ(Utf8Char::ThickLine.c_str(), "\xe2\x94\x81");
  EXPECT_STREQ(Utf8Char::ThinLine.c_str(), "\xe2\x94\x80");
}

TEST_F(Utf8CharTest, ConstantRepeatWorks) {
  std::string result = Utf8Char::ThickLine * 5;
  EXPECT_EQ(result, "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81");
}

}  // namespace sentinel
