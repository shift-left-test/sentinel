// Copyright (c) 2026 LG Electronics Inc.
// SPDX-License-Identifier: MIT

#include "Calculator.hpp"
#include "gtest/gtest.h"

class CalculatorTest : public ::testing::Test {
 protected:
  Calculator calc;
};

// --- Basic arithmetic ---

TEST_F(CalculatorTest, SingleNumber) {
  EXPECT_EQ(calc.evaluate("42"), 42);
}

TEST_F(CalculatorTest, Addition) {
  EXPECT_EQ(calc.evaluate("1 + 2"), 3);
}

TEST_F(CalculatorTest, Subtraction) {
  EXPECT_EQ(calc.evaluate("10 - 3"), 7);
}

TEST_F(CalculatorTest, Multiplication) {
  EXPECT_EQ(calc.evaluate("4 * 5"), 20);
}

TEST_F(CalculatorTest, Division) {
  EXPECT_EQ(calc.evaluate("20 / 4"), 5);
}

// --- Operator precedence ---

TEST_F(CalculatorTest, MultiplicationBeforeAddition) {
  EXPECT_EQ(calc.evaluate("1 + 2 * 3"), 7);
}

TEST_F(CalculatorTest, DivisionBeforeSubtraction) {
  EXPECT_EQ(calc.evaluate("10 - 6 / 2"), 7);
}

TEST_F(CalculatorTest, MixedPrecedence) {
  EXPECT_EQ(calc.evaluate("2 + 3 * 4 - 1"), 13);
}

TEST_F(CalculatorTest, SamePrecedenceLeftToRight) {
  EXPECT_EQ(calc.evaluate("10 - 3 - 2"), 5);
}

TEST_F(CalculatorTest, MultipleDivisionsLeftToRight) {
  EXPECT_EQ(calc.evaluate("100 / 10 / 2"), 5);
}

// --- Parentheses ---

TEST_F(CalculatorTest, ParenthesesOverridePrecedence) {
  EXPECT_EQ(calc.evaluate("(1 + 2) * 3"), 9);
}

TEST_F(CalculatorTest, NestedParentheses) {
  EXPECT_EQ(calc.evaluate("((2 + 3) * (4 - 1))"), 15);
}

TEST_F(CalculatorTest, ParenthesesWithinExpression) {
  EXPECT_EQ(calc.evaluate("1 + 2 * (1 + 2)"), 7);
}

TEST_F(CalculatorTest, DeeplyNestedParentheses) {
  EXPECT_EQ(calc.evaluate("((((1 + 1))))"), 2);
}

// --- Negative numbers ---

TEST_F(CalculatorTest, NegativeNumberAtStart) {
  EXPECT_EQ(calc.evaluate("-5 + 3"), -2);
}

TEST_F(CalculatorTest, NegativeNumberInParentheses) {
  EXPECT_EQ(calc.evaluate("(-5 + 3) * 2"), -4);
}

TEST_F(CalculatorTest, NegativeMultiplied) {
  EXPECT_EQ(calc.evaluate("3 * -2"), -6);
}

// --- Complex expressions ---

TEST_F(CalculatorTest, ComplexExpression1) {
  // 2 * (3 + 4) - 10 / 2 = 14 - 5 = 9
  EXPECT_EQ(calc.evaluate("2 * (3 + 4) - 10 / 2"), 9);
}

TEST_F(CalculatorTest, ComplexExpression2) {
  // (1 + 2) * (3 + 4) = 3 * 7 = 21
  EXPECT_EQ(calc.evaluate("(1 + 2) * (3 + 4)"), 21);
}

TEST_F(CalculatorTest, ComplexExpression3) {
  // 100 / (2 * 5) + 3 * (4 - 1) = 10 + 9 = 19
  EXPECT_EQ(calc.evaluate("100 / (2 * 5) + 3 * (4 - 1)"), 19);
}

TEST_F(CalculatorTest, LargeChainedOperations) {
  // 1 + 2 + 3 + 4 + 5 = 15
  EXPECT_EQ(calc.evaluate("1 + 2 + 3 + 4 + 5"), 15);
}

TEST_F(CalculatorTest, MixedAllOperators) {
  // 10 + 20 * 3 - 50 / 5 = 10 + 60 - 10 = 60
  EXPECT_EQ(calc.evaluate("10 + 20 * 3 - 50 / 5"), 60);
}

// --- Edge cases ---

TEST_F(CalculatorTest, ZeroOperations) {
  EXPECT_EQ(calc.evaluate("0 + 0"), 0);
}

TEST_F(CalculatorTest, MultiplyByZero) {
  EXPECT_EQ(calc.evaluate("999 * 0"), 0);
}

TEST_F(CalculatorTest, SubtractToNegative) {
  EXPECT_EQ(calc.evaluate("3 - 10"), -7);
}

TEST_F(CalculatorTest, IntegerDivisionTruncates) {
  EXPECT_EQ(calc.evaluate("7 / 2"), 3);
}

TEST_F(CalculatorTest, NoSpaces) {
  EXPECT_EQ(calc.evaluate("1+2*3"), 7);
}

// --- Error cases ---

TEST_F(CalculatorTest, DivisionByZeroThrows) {
  EXPECT_THROW(calc.evaluate("10 / 0"), std::runtime_error);
}

TEST_F(CalculatorTest, EmptyExpressionThrows) {
  EXPECT_THROW(calc.evaluate(""), std::runtime_error);
}

TEST_F(CalculatorTest, MismatchedLeftParenThrows) {
  EXPECT_THROW(calc.evaluate("(1 + 2"), std::runtime_error);
}

TEST_F(CalculatorTest, MismatchedRightParenThrows) {
  EXPECT_THROW(calc.evaluate("1 + 2)"), std::runtime_error);
}

TEST_F(CalculatorTest, InvalidCharacterThrows) {
  EXPECT_THROW(calc.evaluate("1 + a"), std::runtime_error);
}
