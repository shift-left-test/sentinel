// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include "Tokenizer.hpp"
#include "gtest/gtest.h"

TEST(TokenizerTest, SingleNumber) {
  Tokenizer t("42");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "42");
}

TEST(TokenizerTest, SimpleAddition) {
  Tokenizer t("1 + 2");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "1");
  EXPECT_EQ(tokens[1].type, TokenType::Operator);
  EXPECT_EQ(tokens[1].value, "+");
  EXPECT_EQ(tokens[2].type, TokenType::Number);
  EXPECT_EQ(tokens[2].value, "2");
}

TEST(TokenizerTest, AllOperators) {
  Tokenizer t("1 + 2 - 3 * 4 / 5");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 9u);
  EXPECT_EQ(tokens[1].value, "+");
  EXPECT_EQ(tokens[3].value, "-");
  EXPECT_EQ(tokens[5].value, "*");
  EXPECT_EQ(tokens[7].value, "/");
}

TEST(TokenizerTest, Parentheses) {
  Tokenizer t("(1 + 2)");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 5u);
  EXPECT_EQ(tokens[0].type, TokenType::LeftParen);
  EXPECT_EQ(tokens[4].type, TokenType::RightParen);
}

TEST(TokenizerTest, MultiDigitNumber) {
  Tokenizer t("123 + 456");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0].value, "123");
  EXPECT_EQ(tokens[2].value, "456");
}

TEST(TokenizerTest, NoSpaces) {
  Tokenizer t("1+2*3");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 5u);
  EXPECT_EQ(tokens[0].value, "1");
  EXPECT_EQ(tokens[1].value, "+");
  EXPECT_EQ(tokens[2].value, "2");
  EXPECT_EQ(tokens[3].value, "*");
  EXPECT_EQ(tokens[4].value, "3");
}

TEST(TokenizerTest, UnaryMinusAtStart) {
  Tokenizer t("-5 + 3");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "-5");
}

TEST(TokenizerTest, UnaryMinusAfterLeftParen) {
  Tokenizer t("(-5 + 3)");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 5u);
  EXPECT_EQ(tokens[1].type, TokenType::Number);
  EXPECT_EQ(tokens[1].value, "-5");
}

TEST(TokenizerTest, UnaryMinusAfterOperator) {
  Tokenizer t("3 * -2");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[2].type, TokenType::Number);
  EXPECT_EQ(tokens[2].value, "-2");
}

TEST(TokenizerTest, InvalidCharacterThrows) {
  Tokenizer t("1 + a");
  EXPECT_THROW(t.tokenize(), std::runtime_error);
}

TEST(TokenizerTest, EmptyExpression) {
  Tokenizer t("");
  auto tokens = t.tokenize();
  EXPECT_TRUE(tokens.empty());
}

TEST(TokenizerTest, WhitespaceOnly) {
  Tokenizer t("   ");
  auto tokens = t.tokenize();
  EXPECT_TRUE(tokens.empty());
}

TEST(TokenizerTest, NestedParentheses) {
  Tokenizer t("((1 + 2))");
  auto tokens = t.tokenize();
  ASSERT_EQ(tokens.size(), 7u);
  EXPECT_EQ(tokens[0].type, TokenType::LeftParen);
  EXPECT_EQ(tokens[1].type, TokenType::LeftParen);
  EXPECT_EQ(tokens[5].type, TokenType::RightParen);
  EXPECT_EQ(tokens[6].type, TokenType::RightParen);
}
