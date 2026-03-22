/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <csignal>
#include <memory>
#include <vector>
#include "sentinel/SignalHandler.hpp"
#include "sentinel/util/signal.hpp"

namespace sentinel {

class SignalHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mSavedHandlers = std::make_unique<signal::SaContainer>(std::vector<int>{SIGUSR1});
    SignalHandler::clear();
  }

  void TearDown() override {
    SignalHandler::clear();
    // mSavedHandlers destructor restores the original SIGUSR1 handler
  }

  std::unique_ptr<signal::SaContainer> mSavedHandlers;
};

TEST_F(SignalHandlerTest, testCallbackInvokedOnSignal) {
  bool called = false;
  SignalHandler::add({SIGUSR1}, [&called]() { called = true; });

  kill(getpid(), SIGUSR1);

  EXPECT_TRUE(called);
}

TEST_F(SignalHandlerTest, testMultipleCallbacksInvokedInRegistrationOrder) {
  std::vector<int> order;
  SignalHandler::add({SIGUSR1}, [&order]() { order.push_back(1); });
  SignalHandler::add({SIGUSR1}, [&order]() { order.push_back(2); });

  kill(getpid(), SIGUSR1);

  ASSERT_EQ(order.size(), 2u);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
}

}  // namespace sentinel
