// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include "Tokenizer.hpp"

#include <cctype>
#include <stdexcept>

Tokenizer::Tokenizer(const std::string& expression)
    : mExpression(expression) {
}

std::vector<Token> Tokenizer::tokenize() {
  std::vector<Token> tokens;
  std::size_t i = 0;

  while (i < mExpression.size()) {
    char ch = mExpression[i];

    if (std::isspace(ch)) {
      ++i;
      continue;
    }

    if (std::isdigit(ch)) {
      std::string num;
      while (i < mExpression.size() && std::isdigit(mExpression[i])) {
        num += mExpression[i];
        ++i;
      }
      tokens.push_back({TokenType::Number, num});
      continue;
    }

    if (ch == '(') {
      tokens.push_back({TokenType::LeftParen, "("});
      ++i;
      continue;
    }

    if (ch == ')') {
      tokens.push_back({TokenType::RightParen, ")"});
      ++i;
      continue;
    }

    if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
      bool isUnaryMinus = (ch == '-') &&
          (tokens.empty() ||
           tokens.back().type == TokenType::Operator ||
           tokens.back().type == TokenType::LeftParen);

      if (isUnaryMinus) {
        ++i;
        if (i >= mExpression.size() || !std::isdigit(mExpression[i])) {
          throw std::runtime_error("Invalid expression: expected digit after unary minus");
        }
        std::string num = "-";
        while (i < mExpression.size() && std::isdigit(mExpression[i])) {
          num += mExpression[i];
          ++i;
        }
        tokens.push_back({TokenType::Number, num});
        continue;
      }

      tokens.push_back({TokenType::Operator, std::string(1, ch)});
      ++i;
      continue;
    }

    throw std::runtime_error(
        std::string("Invalid character: '") + ch + "'");
  }

  return tokens;
}
