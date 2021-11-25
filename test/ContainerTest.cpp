/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
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
  namespace fs = std::experimental::filesystem;
  auto path = fs::temp_directory_path() / "SENTINEL_CONTAINERTEST_TMP_FILE";
  fs::remove_all(path);
  integers.save(path);
  none.load(path);
  EXPECT_EQ(integers, none);
  fs::remove(path);
}

TEST_F(ContainerTest, testSplit) {
  auto splitted = integers.split(0, 2);
  EXPECT_EQ(2, splitted.size());
  EXPECT_EQ(0, splitted[0]);
  EXPECT_EQ(1, splitted[1]);
}

}  // namespace sentinel
