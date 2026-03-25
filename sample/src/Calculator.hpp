// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_SRC_CALCULATOR_HPP_
#define SAMPLE_SRC_CALCULATOR_HPP_

#include <string>
#include <vector>

#include "Stack.hpp"
#include "Tokenizer.hpp"

class Calculator {
 public:
  int evaluate(const std::string& expression);

 private:
  static int precedence(char op);
  static int applyOp(int lhs, int rhs, char op);
  static void processTop(Stack& values, std::vector<char>& ops);
};

#endif  // SAMPLE_SRC_CALCULATOR_HPP_
