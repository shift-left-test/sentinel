/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_HELPER_THROWMESSAGEMATCHER_HPP_
#define TEST_INCLUDE_HELPER_THROWMESSAGEMATCHER_HPP_

#include <gmock/gmock.h>

/**
 * @brief Verify that @p statement throws @p exception_type whose what()
 *        matches @p matcher.
 *
 * Drop-in replacement for GoogleTest 1.12+ ThrowsMessage that works with
 * GoogleTest 1.10.
 */
#define EXPECT_THROW_MESSAGE(statement, exception_type, matcher) \
  try {                                                          \
    statement;                                                   \
    FAIL() << "Expected " #exception_type " but no exception";  \
  } catch (const exception_type& e) {                           \
    EXPECT_THAT(e.what(), matcher);                              \
  } catch (...) {                                                \
    FAIL() << "Expected " #exception_type " but got different";  \
  }

#endif  // TEST_INCLUDE_HELPER_THROWMESSAGEMATCHER_HPP_
