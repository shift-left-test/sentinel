/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

class StringTest : public ::testing::Test {};

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
  EXPECT_STREQ(HELLO_WORLD, string::trim(SPACE_HELLO_WORLD_SPACE).c_str());
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
  std::vector<std::string> expected = {HELLO, WORLD};
  EXPECT_EQ(expected, string::split(HELLO_WORLD));
}

TEST_F(StringTest, testSplitByStringDeliReturnSplittedTokens) {
  std::vector<std::string> expected = {HELLO, "ORLD"};
  EXPECT_EQ(expected, string::split(HELLO_WORLD, " W"));
}

TEST_F(StringTest, testJoinReturnJoinedCharactersWhenVectorStrGiven) {
  std::vector<std::string> input = {HELLO, WORLD};
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
  EXPECT_STREQ(HELLO_HELLO, string::replaceAll(HELLO_WORLD, WORLD, HELLO).c_str());
  EXPECT_STREQ(HELLO_HELLO, string::replaceAll(WORLD_WORLD, WORLD, HELLO).c_str());
}

TEST_F(StringTest, testToLowerConvertsUppercaseToLowercase) {
  EXPECT_EQ("hello world", string::toLower("HELLO WORLD"));
  EXPECT_EQ("hello world", string::toLower("Hello World"));
  EXPECT_EQ("hello world", string::toLower("hello world"));
  EXPECT_EQ("", string::toLower(""));
  EXPECT_EQ("abc123", string::toLower("ABC123"));
}

TEST_F(StringTest, testToUpperConvertsLowercaseToUppercase) {
  EXPECT_EQ("HELLO WORLD", string::toUpper("hello world"));
  EXPECT_EQ("HELLO WORLD", string::toUpper("Hello World"));
  EXPECT_EQ("HELLO WORLD", string::toUpper("HELLO WORLD"));
  EXPECT_EQ("", string::toUpper(""));
  EXPECT_EQ("ABC123", string::toUpper("abc123"));
}

TEST_F(StringTest, testStartsWithReturnsFalseWhenNeedleLongerThanHaystack) {
  EXPECT_FALSE(string::startsWith("Hi", "Hello"));
  EXPECT_FALSE(string::startsWith("", "x"));
}

TEST_F(StringTest, testEndsWithReturnsFalseWhenNeedleLongerThanHaystack) {
  EXPECT_FALSE(string::endsWith("Hi", "Hello"));
  EXPECT_FALSE(string::endsWith("", "x"));
}

TEST_F(StringTest, testLtrimWithCustomPredicate) {
  auto pred = [](unsigned char c) { return c == 'x'; };
  EXPECT_EQ("hello", string::ltrim("xxxhello", pred));
  EXPECT_EQ("hello", string::ltrim("hello", pred));
  EXPECT_EQ("", string::ltrim("xxx", pred));
  EXPECT_EQ("", string::ltrim("", pred));
}

TEST_F(StringTest, testRtrimWithCustomPredicate) {
  auto pred = [](unsigned char c) { return c == 'x'; };
  EXPECT_EQ("hello", string::rtrim("helloxxx", pred));
  EXPECT_EQ("hello", string::rtrim("hello", pred));
  EXPECT_EQ("", string::rtrim("xxx", pred));
  EXPECT_EQ("", string::rtrim("", pred));
}

TEST_F(StringTest, testTrimWithCustomPredicate) {
  auto pred = [](unsigned char c) { return c == '#'; };
  EXPECT_EQ("hello", string::trim("##hello##", pred));
  EXPECT_EQ("", string::trim("####", pred));
}

TEST_F(StringTest, testSplitWithCharDelimiterEdgeCases) {
  auto empty = string::split("", ' ');
  EXPECT_TRUE(empty.empty());

  auto noDelim = string::split("abc", ',');
  ASSERT_EQ(1u, noDelim.size());
  EXPECT_EQ("abc", noDelim[0]);

  auto twoTokens = string::split("a,b", ',');
  ASSERT_EQ(2u, twoTokens.size());
  EXPECT_EQ("a", twoTokens[0]);
  EXPECT_EQ("b", twoTokens[1]);

  auto middleDelim = string::split("a,,b", ',');
  ASSERT_EQ(3u, middleDelim.size());
  EXPECT_EQ("a", middleDelim[0]);
  EXPECT_EQ("", middleDelim[1]);
  EXPECT_EQ("b", middleDelim[2]);
}

TEST_F(StringTest, testSplitByStringDelimiterEdgeCases) {
  auto noMatch = string::split("hello", "::");
  ASSERT_EQ(1u, noMatch.size());
  EXPECT_EQ("hello", noMatch[0]);

  auto startMatch = string::split("::hello", "::");
  ASSERT_EQ(2u, startMatch.size());
  EXPECT_EQ("", startMatch[0]);
  EXPECT_EQ("hello", startMatch[1]);

  auto endMatch = string::split("hello::", "::");
  ASSERT_EQ(2u, endMatch.size());
  EXPECT_EQ("hello", endMatch[0]);
  EXPECT_EQ("", endMatch[1]);

  auto multiMatch = string::split("a::b::c", "::");
  ASSERT_EQ(3u, multiMatch.size());
  EXPECT_EQ("a", multiMatch[0]);
  EXPECT_EQ("b", multiMatch[1]);
  EXPECT_EQ("c", multiMatch[2]);
}

TEST_F(StringTest, testJoinWithEmptyVector) {
  std::vector<std::string> empty;
  EXPECT_EQ("", string::join(",", empty));
}

TEST_F(StringTest, testJoinWithSingleElement) {
  std::vector<std::string> single = {"only"};
  EXPECT_EQ("only", string::join(",", single));
}

TEST_F(StringTest, testJoinWithCharDelimiter) {
  std::vector<std::string> items = {"a", "b", "c"};
  EXPECT_EQ("a,b,c", string::join(',', items));
}

TEST_F(StringTest, testJoinVariadicWithCharDelimiter) {
  EXPECT_EQ("a,b,c", string::join(',', "a", "b", "c"));
}

TEST_F(StringTest, testReplaceAllWithOldStrEmpty) {
  EXPECT_EQ("hello", string::replaceAll("hello", "", "x"));
}

TEST_F(StringTest, testReplaceAllNoMatch) {
  EXPECT_EQ("hello", string::replaceAll("hello", "xyz", "abc"));
}

TEST_F(StringTest, testToIntValid) {
  EXPECT_EQ(42, string::to<int>("42"));
  EXPECT_EQ(42, string::to<int>("+42"));
  EXPECT_EQ(42, string::to<int>("  42  "));
  EXPECT_EQ(0, string::to<int>("0"));
}

TEST_F(StringTest, testToIntThrowsOnEmpty) {
  EXPECT_THROW(string::to<int>(""), InvalidArgumentException);
  EXPECT_THROW(string::to<int>("   "), InvalidArgumentException);
}

TEST_F(StringTest, testToIntThrowsOnNonNumeric) {
  EXPECT_THROW(string::to<int>("abc"), InvalidArgumentException);
  EXPECT_THROW(string::to<int>("12abc"), InvalidArgumentException);
  EXPECT_THROW(string::to<int>("12.5"), InvalidArgumentException);
}

TEST_F(StringTest, testToSizeT) {
  EXPECT_EQ(100u, string::to<size_t>("100"));
  EXPECT_EQ(0u, string::to<size_t>("0"));
}

TEST_F(StringTest, testToBoolValid) {
  EXPECT_TRUE(string::to<bool>("true"));
  EXPECT_FALSE(string::to<bool>("false"));
}

TEST_F(StringTest, testToBoolThrowsOnInvalid) {
  EXPECT_THROW(string::to<bool>("True"), InvalidArgumentException);
  EXPECT_THROW(string::to<bool>("FALSE"), InvalidArgumentException);
  EXPECT_THROW(string::to<bool>("1"), InvalidArgumentException);
  EXPECT_THROW(string::to<bool>(""), InvalidArgumentException);
  EXPECT_THROW(string::to<bool>("yes"), InvalidArgumentException);
}

TEST_F(StringTest, testFrom) {
  EXPECT_STREQ("true", string::from(true));
  EXPECT_STREQ("false", string::from(false));
}

TEST_F(StringTest, testTruncateNoTruncationNeeded) {
  EXPECT_EQ("short", string::truncate("short", 10));
  EXPECT_EQ("exact", string::truncate("exact", 5));
}

TEST_F(StringTest, testTruncateWithEllipsis) {
  EXPECT_EQ("...world", string::truncate("hello world", 8));
}

TEST_F(StringTest, testTruncateVeryShortMaxLen) {
  EXPECT_EQ("d", string::truncate("hello world", 1));
  EXPECT_EQ("ld", string::truncate("hello world", 2));
  EXPECT_EQ("rld", string::truncate("hello world", 3));
}

}  // namespace sentinel
