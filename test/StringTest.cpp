/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include "sentinel/util/string.hpp"


namespace sentinel {

class StringTest : public ::testing::Test {
};

static constexpr const char* SPACE_HELLO_WORLD_SPACE = " HELLO WORLD ";
static constexpr const char* SPACE_HELLO_WORLD = " HELLO WORLD";
static constexpr const char* HELLO_HELLO = "HELLO HELLO";
static constexpr const char* HELLO_WORLD_SPACE = "HELLO WORLD ";
static constexpr const char* HELLO_WORLD = "HELLO WORLD";
static constexpr const char* hello_world = "hello world";
static constexpr const char* HELLO = "HELLO";
static constexpr const char* WORLD = "WORLD";
static constexpr const char* WORLD_WORLD = "WORLD WORLD";
static constexpr const char* SPACE = " ";
static constexpr const char* BLANK = "";

TEST_F(StringTest, testStartsWithReturnTrueWhenValidArgsGiven) {
  EXPECT_TRUE(string::startsWith(HELLO_WORLD, HELLO_WORLD));
  EXPECT_TRUE(string::startsWith(HELLO_WORLD, HELLO));
  EXPECT_TRUE(string::startsWith(HELLO_WORLD, BLANK));
  EXPECT_TRUE(string::startsWith(SPACE, SPACE));
  EXPECT_TRUE(string::startsWith(BLANK, BLANK));
}

TEST_F(StringTest, testStartsWithReturnFalseWhenInvalidArgsGiven) {
  EXPECT_FALSE(string::startsWith(HELLO_WORLD, WORLD));
  EXPECT_FALSE(string::startsWith(HELLO_WORLD, SPACE));
  EXPECT_FALSE(string::startsWith(HELLO, WORLD));
}

TEST_F(StringTest, testEndsWithReturnTrueWhenValidArgsGiven) {
  EXPECT_TRUE(string::endsWith(HELLO_WORLD, HELLO_WORLD));
  EXPECT_TRUE(string::endsWith(HELLO_WORLD, WORLD));
  EXPECT_TRUE(string::endsWith(HELLO_WORLD, BLANK));
  EXPECT_TRUE(string::endsWith(SPACE, SPACE));
  EXPECT_TRUE(string::endsWith(BLANK, BLANK));
}

TEST_F(StringTest, testEndsWithReturnFalseWhenInvalidArgsGiven) {
  EXPECT_FALSE(string::endsWith(HELLO_WORLD, HELLO));
  EXPECT_FALSE(string::endsWith(HELLO_WORLD, SPACE));
  EXPECT_FALSE(string::endsWith(HELLO, WORLD));
}

TEST_F(StringTest, testLtrimShouldTrimLeadingWhitespaces) {
  EXPECT_STREQ(HELLO_WORLD, string::ltrim(SPACE_HELLO_WORLD).c_str());
  EXPECT_STREQ(HELLO_WORLD, string::ltrim(HELLO_WORLD).c_str());
  EXPECT_STREQ(BLANK, string::ltrim(SPACE).c_str());
  EXPECT_STREQ(BLANK, string::ltrim(BLANK).c_str());
}

TEST_F(StringTest, testLtrimShouldNotTrimTrailingWhitespaces) {
  EXPECT_STRNE(HELLO_WORLD, string::ltrim(HELLO_WORLD_SPACE).c_str());
}

TEST_F(StringTest, testRtrimShouldTrimTrailingWhitespaces) {
  EXPECT_STREQ(HELLO_WORLD, string::rtrim(HELLO_WORLD_SPACE).c_str());
  EXPECT_STREQ(HELLO_WORLD, string::rtrim(HELLO_WORLD).c_str());
  EXPECT_STREQ(BLANK, string::rtrim(SPACE).c_str());
  EXPECT_STREQ(BLANK, string::rtrim(BLANK).c_str());
}

TEST_F(StringTest, testRtrimShouldNotTrimLeadingWhitespaces) {
  EXPECT_STRNE(HELLO_WORLD, string::rtrim(SPACE_HELLO_WORLD).c_str());
}

TEST_F(StringTest, testTrimShouldTrimLeadingAndTrailingWhitespaces) {
  EXPECT_STREQ(HELLO_WORLD,
               string::trim(SPACE_HELLO_WORLD_SPACE).c_str());
  EXPECT_STREQ(HELLO_WORLD, string::trim(SPACE_HELLO_WORLD).c_str());
  EXPECT_STREQ(HELLO_WORLD, string::trim(HELLO_WORLD_SPACE).c_str());
  EXPECT_STREQ(BLANK, string::trim(SPACE).c_str());
  EXPECT_STREQ(BLANK, string::trim(BLANK).c_str());
}

TEST_F(StringTest, testContainsReturnTrueWhenValidArgsGiven) {
  EXPECT_TRUE(string::contains(SPACE_HELLO_WORLD_SPACE, HELLO_WORLD));
  EXPECT_TRUE(string::contains(HELLO_WORLD, HELLO));
  EXPECT_TRUE(string::contains(HELLO_WORLD, WORLD));
  EXPECT_TRUE(string::contains(HELLO_WORLD, SPACE));
  EXPECT_TRUE(string::contains(HELLO_WORLD, BLANK));
}

TEST_F(StringTest, testContainsReturnFalseWhenInvalidArgsGiven) {
  EXPECT_FALSE(string::contains(HELLO_WORLD, SPACE_HELLO_WORLD_SPACE));
  EXPECT_FALSE(string::contains(HELLO_WORLD, hello_world));
  EXPECT_FALSE(string::contains(HELLO, WORLD));
  EXPECT_FALSE(string::contains(SPACE, HELLO_WORLD));
  EXPECT_FALSE(string::contains(BLANK, HELLO_WORLD));
}

TEST_F(StringTest, testSplitReturnSplittedTokens) {
  std::vector<std::string> expected = { HELLO, WORLD };
  EXPECT_EQ(expected, string::split(HELLO_WORLD));
}

TEST_F(StringTest, testSplitByStringDeliReturnSplittedTokens) {
  std::vector<std::string> expected = { HELLO, "ORLD" };
  EXPECT_EQ(expected, string::split(HELLO_WORLD, " W"));
}

TEST_F(StringTest, testJoinReturnJoinedCharactersWhenVectorStrGiven) {
  std::vector<std::string> input = { HELLO, WORLD };
  EXPECT_STREQ(HELLO_WORLD, string::join(SPACE, input).c_str());
}

TEST_F(StringTest, testJoinReturnJoinedCharactersWhenVariadicArgsGiven) {
  EXPECT_STREQ("a", string::join("", "a").c_str());
  EXPECT_STREQ("ab", string::join("", "a", "b").c_str());
  EXPECT_STREQ("abc", string::join("", "a", "b", "c").c_str());
}

TEST_F(StringTest, testReplaceAll) {
  EXPECT_STREQ(BLANK, string::replaceAll(BLANK, BLANK, "b").c_str());
  EXPECT_STREQ(BLANK, string::replaceAll(WORLD, WORLD, BLANK).c_str());
  EXPECT_STREQ(WORLD, string::replaceAll(WORLD, "A", "B").c_str());
  EXPECT_STREQ(HELLO_HELLO, string::replaceAll(
      HELLO_WORLD, WORLD, HELLO).c_str());
  EXPECT_STREQ(HELLO_HELLO, string::replaceAll(
      WORLD_WORLD, WORLD, HELLO).c_str());
}

}  // namespace sentinel
