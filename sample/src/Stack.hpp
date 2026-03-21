// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_SRC_STACK_HPP_
#define SAMPLE_SRC_STACK_HPP_

#include <cstddef>
#include <vector>

class Stack {
 public:
  void push(int value);
  int pop();
  int peek() const;
  bool isEmpty() const;
  std::size_t size() const;

 private:
  std::vector<int> mData;
};

#endif  // SAMPLE_SRC_STACK_HPP_
