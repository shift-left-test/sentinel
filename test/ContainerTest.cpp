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
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include "sentinel/util/os.hpp"
#include "sentinel/Container.hpp"


namespace sentinel {

class Integers : public Container<int> {
 public:
  bool operator==(const Integers& other) const {
    return std::equal(begin(), end(), other.begin());
  }

  bool operator!=(const Integers& other) const {
    return !std::equal(begin(), end(), other.begin());
  }
};

class ContainerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    for (auto i = 0; i < N; i++) {
      integers.push_back(i);
    }
  }

  Integers integers;
  Integers none;
  const std::size_t N = 10;
};

TEST_F(ContainerTest, testSize) {
  EXPECT_EQ(0, none.size());
  EXPECT_EQ(N, integers.size());
}

TEST_F(ContainerTest, testIterators) {
  EXPECT_EQ(none.begin(), none.end());
  EXPECT_EQ(N, std::distance(integers.begin(), integers.end()));
}

TEST_F(ContainerTest, testAt) {
  EXPECT_THROW(none.at(0), std::out_of_range);
  EXPECT_THROW(none.at(N), std::out_of_range);
  EXPECT_EQ(0, integers.at(0));
  EXPECT_EQ(N - 1, integers.at(N - 1));
  EXPECT_THROW(integers.at(N), std::out_of_range);
}

TEST_F(ContainerTest, testSubscriptOperator) {
  EXPECT_THROW(none[0], std::out_of_range);
  EXPECT_THROW(none[N], std::out_of_range);
  EXPECT_EQ(0, integers[0]);
  EXPECT_EQ(N - 1, integers[N - 1]);
  EXPECT_THROW(integers[N], std::out_of_range);
}

TEST_F(ContainerTest, testPushback) {
  none.push_back(1);
  none.push_back(1);
  EXPECT_EQ(2, none.size());
}

TEST_F(ContainerTest, testEmplaceback) {
  none.emplace_back(1);
  none.emplace_back(1);
  EXPECT_EQ(2, none.size());
}

TEST_F(ContainerTest, testSortInDescendingOrder) {
  none.emplace_back(1);
  none.emplace_back(2);
  none.sort(std::greater<>());
  EXPECT_EQ(2, none[0]);
  EXPECT_EQ(1, none[1]);
}

TEST_F(ContainerTest, testSort) {
  none.emplace_back(2);
  none.emplace_back(1);
  none.sort();
  EXPECT_EQ(1, none[0]);
  EXPECT_EQ(2, none[1]);
}

TEST_F(ContainerTest, testShuffle) {
  auto shuffled = integers;
  shuffled.shuffle();
  EXPECT_NE(integers, shuffled);
}

TEST_F(ContainerTest, testClear) {
  EXPECT_EQ(N, integers.size());
  integers.clear();
  EXPECT_EQ(0, integers.size());
}

TEST_F(ContainerTest, testSaveAndLoad) {
  auto path = os::tempPath();
  integers.save(path);
  none.load(path);
  EXPECT_EQ(integers, none);
  os::removeFile(path);
}

}  // namespace sentinel
