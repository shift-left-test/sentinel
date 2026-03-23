// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include <stdexcept>
#include "Stack.hpp"

void Stack::push(int value) {
  mData.push_back(value);
}

int Stack::pop() {
  if (isEmpty()) {
    throw std::runtime_error("Stack is empty");
  }
  int top = mData.back();
  mData.pop_back();
  return top;
}

int Stack::peek() const {
  if (isEmpty()) {
    throw std::runtime_error("Stack is empty");
  }
  return mData.back();
}

bool Stack::isEmpty() const {
  return mData.empty();
}

std::size_t Stack::size() const {
  return mData.size();
}
