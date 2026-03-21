// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include "Stack.hpp"
#include "gtest/gtest.h"

TEST(StackTest, NewStackIsEmpty) {
  Stack s;
  EXPECT_TRUE(s.isEmpty());
  EXPECT_EQ(s.size(), 0u);
}

TEST(StackTest, PushIncreasesSize) {
  Stack s;
  s.push(1);
  EXPECT_FALSE(s.isEmpty());
  EXPECT_EQ(s.size(), 1u);
}

TEST(StackTest, PeekReturnsTopWithoutRemoving) {
  Stack s;
  s.push(42);
  EXPECT_EQ(s.peek(), 42);
  EXPECT_EQ(s.size(), 1u);
}

TEST(StackTest, PopReturnsTopAndRemovesIt) {
  Stack s;
  s.push(10);
  s.push(20);
  EXPECT_EQ(s.pop(), 20);
  EXPECT_EQ(s.size(), 1u);
}

TEST(StackTest, PopOnEmptyThrows) {
  Stack s;
  EXPECT_THROW(s.pop(), std::runtime_error);
}

TEST(StackTest, PeekOnEmptyThrows) {
  Stack s;
  EXPECT_THROW(s.peek(), std::runtime_error);
}

TEST(StackTest, PushPopMultipleElements) {
  Stack s;
  s.push(1);
  s.push(2);
  s.push(3);
  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);
  EXPECT_TRUE(s.isEmpty());
}
