/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <cerrno>
#include <stdexcept>
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

class ExceptionTest : public ::testing::Test {
};

TEST_F(ExceptionTest, testIOExceptionType) {
  EXPECT_THROW(throw IOException(EEXIST), std::runtime_error);
}

TEST_F(ExceptionTest, testIOExceptionMessage) {
  EXPECT_STREQ("File exists", IOException(EEXIST).what());
  EXPECT_STREQ("Operation not permitted", IOException(EPERM).what());
}

}  // namespace sentinel
