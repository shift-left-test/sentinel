/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <cerrno>
#include <stdexcept>
#include "sentinel/MutationState.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/exceptions/RepositoryException.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"

namespace sentinel {

class ExceptionTest : public ::testing::Test {};

TEST_F(ExceptionTest, testIOExceptionType) {
  EXPECT_THROW(throw IOException(EEXIST), std::runtime_error);
}

TEST_F(ExceptionTest, testIOExceptionMessage) {
  EXPECT_STREQ("File exists", IOException(EEXIST).what());
  EXPECT_STREQ("Operation not permitted", IOException(EPERM).what());
}

TEST_F(ExceptionTest, testInvalidArgumentExceptionType) {
  EXPECT_THROW(throw InvalidArgumentException("test"), std::invalid_argument);
}

TEST_F(ExceptionTest, testInvalidArgumentExceptionMessage) {
  EXPECT_STREQ("bad argument", InvalidArgumentException("bad argument").what());
}

TEST_F(ExceptionTest, testRepositoryExceptionType) {
  EXPECT_THROW(throw RepositoryException("test"), std::runtime_error);
}

TEST_F(ExceptionTest, testRepositoryExceptionMessage) {
  EXPECT_STREQ("repo error", RepositoryException("repo error").what());
}

TEST_F(ExceptionTest, testThresholdErrorType) {
  EXPECT_THROW(throw ThresholdError(60.0, 80.0), std::runtime_error);
}

TEST_F(ExceptionTest, testThresholdErrorMessage) {
  EXPECT_STREQ("Mutation score 60.0% is below threshold 80.0%", ThresholdError(60.0, 80.0).what());
}

class MutationStateTest : public ::testing::Test {};

TEST_F(MutationStateTest, testMutationStateToStrReturnsCorrectStrings) {
  EXPECT_STREQ("KILLED", MutationStateToStr(MutationState::KILLED));
  EXPECT_STREQ("SURVIVED", MutationStateToStr(MutationState::SURVIVED));
  EXPECT_STREQ("RUNTIME_ERROR", MutationStateToStr(MutationState::RUNTIME_ERROR));
  EXPECT_STREQ("BUILD_FAILURE", MutationStateToStr(MutationState::BUILD_FAILURE));
  EXPECT_STREQ("TIMEOUT", MutationStateToStr(MutationState::TIMEOUT));
}

}  // namespace sentinel
