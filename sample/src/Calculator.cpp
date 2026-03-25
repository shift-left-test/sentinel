// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include "Calculator.hpp"

#include <stdexcept>

int Calculator::precedence(char op) {
  if (op == '+' || op == '-') {
    return 1;
  }
  if (op == '*' || op == '/') {
    return 2;
  }
  return 0;
}

int Calculator::applyOp(int lhs, int rhs, char op) {
  switch (op) {
    case '+': return lhs + rhs;
    case '-': return lhs - rhs;
    case '*': return lhs * rhs;
    case '/':
      if (rhs == 0) {
        throw std::runtime_error("Division by zero");
      }
      return lhs / rhs;
    default:
      throw std::runtime_error(
          std::string("Unknown operator: '") + op + "'");
  }
}

void Calculator::processTop(Stack& values, std::vector<char>& ops) {
  if (values.size() < 2) {
    throw std::runtime_error("Invalid expression");
  }
  int rhs = values.pop();
  int lhs = values.pop();
  char op = ops.back();
  ops.pop_back();
  values.push(applyOp(lhs, rhs, op));
}

int Calculator::evaluate(const std::string& expression) {
  Tokenizer tokenizer(expression);
  auto tokens = tokenizer.tokenize();

  if (tokens.empty()) {
    throw std::runtime_error("Empty expression");
  }

  Stack values;
  std::vector<char> ops;

  for (const auto& token : tokens) {
    switch (token.type) {
      case TokenType::Number:
        values.push(std::stoi(token.value));
        break;

      case TokenType::LeftParen:
        ops.push_back('(');
        break;

      case TokenType::RightParen: {
        while (!ops.empty() && ops.back() != '(') {
          processTop(values, ops);
        }
        if (ops.empty()) {
          throw std::runtime_error("Mismatched parentheses");
        }
        ops.pop_back();  // remove '('
        break;
      }

      case TokenType::Operator: {
        char currentOp = token.value[0];
        while (!ops.empty() && ops.back() != '(' &&
               precedence(ops.back()) >= precedence(currentOp)) {
          processTop(values, ops);
        }
        ops.push_back(currentOp);
        break;
      }
    }
  }

  while (!ops.empty()) {
    if (ops.back() == '(') {
      throw std::runtime_error("Mismatched parentheses");
    }
    processTop(values, ops);
  }

  if (values.size() != 1) {
    throw std::runtime_error("Invalid expression");
  }

  return values.pop();
}
