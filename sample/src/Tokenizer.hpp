// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_SRC_TOKENIZER_HPP_
#define SAMPLE_SRC_TOKENIZER_HPP_

#include <string>
#include <vector>

enum class TokenType {
  Number,
  Operator,
  LeftParen,
  RightParen
};

struct Token {
  TokenType type;
  std::string value;
};

class Tokenizer {
 public:
  explicit Tokenizer(const std::string& expression);
  std::vector<Token> tokenize();

 private:
  std::string mExpression;
};

#endif  // SAMPLE_SRC_TOKENIZER_HPP_
