/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <llvm/Support/ErrorHandling.h>
#include <new>
#include "sentinel/OomHandler.hpp"

namespace sentinel {

// installOomHandlers() ends with _exit(137) on the actual OOM path, which
// would terminate the test binary; these tests verify only the install
// side. TearDown restores the LLVM defaults so subsequent tests run clean.

class OomHandlerTest : public ::testing::Test {
 protected:
  void TearDown() override {
    llvm::remove_bad_alloc_error_handler();
    llvm::remove_fatal_error_handler();
    std::set_new_handler(nullptr);
  }
};

TEST_F(OomHandlerTest, testInstallSetsNewHandler) {
  std::set_new_handler(nullptr);
  ASSERT_EQ(std::get_new_handler(), nullptr);

  installOomHandlers();

  EXPECT_NE(std::get_new_handler(), nullptr);
}

TEST_F(OomHandlerTest, testNewHandlerIsStableAcrossInstalls) {
  installOomHandlers();
  std::new_handler first = std::get_new_handler();
  installOomHandlers();
  std::new_handler second = std::get_new_handler();

  EXPECT_NE(first, nullptr);
  EXPECT_EQ(first, second);
}

}  // namespace sentinel
