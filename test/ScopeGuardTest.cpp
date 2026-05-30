/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include "sentinel/util/ScopeGuard.hpp"

namespace sentinel {

class ScopeGuardTest : public ::testing::Test {};

TEST_F(ScopeGuardTest, testInvokesCallbackOnDestruction) {
  bool called = false;
  {
    ScopeGuard guard{[&] { called = true; }};
    EXPECT_FALSE(called);
  }
  EXPECT_TRUE(called);
}

TEST_F(ScopeGuardTest, testSwallowsExceptionsFromCallback) {
  // The destructor is implicitly noexcept; a throwing callback that escaped it
  // would call std::terminate and abort this test. Passing without aborting
  // proves the guard swallows cleanup failures (best-effort semantics).
  EXPECT_NO_THROW({
    ScopeGuard guard{[] { throw std::runtime_error("cleanup failed"); }};
  });
}

}  // namespace sentinel
