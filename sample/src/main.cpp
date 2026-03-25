// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include <iostream>
#include <string>

#include "Calculator.hpp"

int main(int argc, char* argv[]) {
  std::string expression;

  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      if (i > 1) {
        expression += ' ';
      }
      expression += argv[i];
    }
  } else {
    std::cout << "Expression: ";
    std::getline(std::cin, expression);
  }

  try {
    Calculator calc;
    int result = calc.evaluate(expression);
    std::cout << result << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
