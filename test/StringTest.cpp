/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <gtest/gtest.h>
#include <string>
#include "sentinel/util/string.hpp"


namespace sentinel {

class StringTest : public ::testing::Test {
 protected:
  static constexpr const char* SPACE_HELLO_WORLD_SPACE = " HELLO WORLD ";
  static constexpr const char* SPACE_HELLO_WORLD = " HELLO WORLD";
  static constexpr const char* HELLO_WORLD_SPACE = "HELLO WORLD ";
  static constexpr const char* HELLO_WORLD = "HELLO WORLD";
  static constexpr const char* hello_world = "hello world";
  static constexpr const char* HELLO = "HELLO";
  static constexpr const char* WORLD = "WORLD";
  static constexpr const char* SPACE = " ";
  static constexpr const char* BLANK = "";
};

TEST_F(StringTest, testStartsWith) {
  EXPECT_TRUE(util::string::startsWith(HELLO_WORLD, HELLO_WORLD));
  EXPECT_TRUE(util::string::startsWith(HELLO_WORLD, HELLO));
  EXPECT_TRUE(util::string::startsWith(HELLO_WORLD, BLANK));
  EXPECT_TRUE(util::string::startsWith(SPACE, SPACE));
  EXPECT_TRUE(util::string::startsWith(BLANK, BLANK));

  EXPECT_FALSE(util::string::startsWith(HELLO_WORLD, WORLD));
  EXPECT_FALSE(util::string::startsWith(HELLO_WORLD, SPACE));
  EXPECT_FALSE(util::string::startsWith(HELLO, WORLD));
}

TEST_F(StringTest, testEndsWith) {
  EXPECT_TRUE(util::string::endsWith(HELLO_WORLD, HELLO_WORLD));
  EXPECT_TRUE(util::string::endsWith(HELLO_WORLD, WORLD));
  EXPECT_TRUE(util::string::endsWith(HELLO_WORLD, BLANK));
  EXPECT_TRUE(util::string::endsWith(SPACE, SPACE));
  EXPECT_TRUE(util::string::endsWith(BLANK, BLANK));

  EXPECT_FALSE(util::string::endsWith(HELLO_WORLD, HELLO));
  EXPECT_FALSE(util::string::endsWith(HELLO_WORLD, SPACE));
  EXPECT_FALSE(util::string::endsWith(HELLO, WORLD));
}

TEST_F(StringTest, testLtrim) {
  EXPECT_STREQ(HELLO_WORLD, util::string::ltrim(SPACE_HELLO_WORLD).c_str());
  EXPECT_STREQ(HELLO_WORLD, util::string::ltrim(HELLO_WORLD).c_str());
  EXPECT_STREQ(BLANK, util::string::ltrim(SPACE).c_str());
  EXPECT_STREQ(BLANK, util::string::ltrim(BLANK).c_str());

  EXPECT_STRNE(HELLO_WORLD, util::string::ltrim(HELLO_WORLD_SPACE).c_str());
}

TEST_F(StringTest, testRtrim) {
  EXPECT_STREQ(HELLO_WORLD, util::string::rtrim(HELLO_WORLD_SPACE).c_str());
  EXPECT_STREQ(HELLO_WORLD, util::string::rtrim(HELLO_WORLD).c_str());
  EXPECT_STREQ(BLANK, util::string::rtrim(SPACE).c_str());
  EXPECT_STREQ(BLANK, util::string::rtrim(BLANK).c_str());

  EXPECT_STRNE(HELLO_WORLD, util::string::rtrim(SPACE_HELLO_WORLD).c_str());
}

TEST_F(StringTest, testTrim) {
  EXPECT_STREQ(HELLO_WORLD,
               util::string::trim(SPACE_HELLO_WORLD_SPACE).c_str());
  EXPECT_STREQ(HELLO_WORLD, util::string::trim(SPACE_HELLO_WORLD).c_str());
  EXPECT_STREQ(HELLO_WORLD, util::string::trim(HELLO_WORLD_SPACE).c_str());
  EXPECT_STREQ(BLANK, util::string::trim(SPACE).c_str());
  EXPECT_STREQ(BLANK, util::string::trim(BLANK).c_str());
}

TEST_F(StringTest, testContains) {
  EXPECT_TRUE(util::string::contains(SPACE_HELLO_WORLD_SPACE, HELLO_WORLD));
  EXPECT_TRUE(util::string::contains(HELLO_WORLD, HELLO));
  EXPECT_TRUE(util::string::contains(HELLO_WORLD, WORLD));
  EXPECT_TRUE(util::string::contains(HELLO_WORLD, SPACE));
  EXPECT_TRUE(util::string::contains(HELLO_WORLD, BLANK));

  EXPECT_FALSE(util::string::contains(HELLO_WORLD, SPACE_HELLO_WORLD_SPACE));
  EXPECT_FALSE(util::string::contains(HELLO_WORLD, hello_world));
  EXPECT_FALSE(util::string::contains(HELLO, WORLD));
  EXPECT_FALSE(util::string::contains(SPACE, HELLO_WORLD));
  EXPECT_FALSE(util::string::contains(BLANK, HELLO_WORLD));
}

TEST_F(StringTest, testFormat) {
  EXPECT_STREQ(HELLO_WORLD,
               util::string::format("%s %s", HELLO, WORLD).c_str());
  EXPECT_STREQ("12.0cde",
               util::string::format("%d%.1f%c%s", 1, 2.0, 'c', "de").c_str());
}

TEST_F(StringTest, testSplit) {
  std::vector<std::string> expected = { HELLO, WORLD };
  EXPECT_EQ(expected, util::string::split(HELLO_WORLD));
}

TEST_F(StringTest, testJoin) {
  std::vector<std::string> input = { HELLO, WORLD };
  EXPECT_STREQ(HELLO_WORLD, util::string::join(input, SPACE).c_str());
}

}  // namespace sentinel
