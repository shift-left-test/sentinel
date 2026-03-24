/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/StatusLine.hpp"

namespace sentinel {

class StatusLineTest : public ::testing::Test {};

TEST_F(StatusLineTest, testEnableIsNopWhenNotTTY) {
  // In non-TTY test env, enable() is a no-op; no crash expected
  StatusLine sl;
  EXPECT_NO_THROW(sl.enable());
  EXPECT_NO_THROW(sl.disable());  // idempotent disable after nop enable
}

TEST_F(StatusLineTest, testDisableIsIdempotent) {
  StatusLine sl;
  EXPECT_NO_THROW(sl.disable());
  EXPECT_NO_THROW(sl.disable());
}

TEST_F(StatusLineTest, testSetPhaseDoesNotCrash) {
  StatusLine sl;
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::INIT));
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::BUILD_ORIG));
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::TEST_ORIG));
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::GENERATION));
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::EVALUATION));
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::REPORT));
  EXPECT_NO_THROW(sl.setPhase(StatusLine::Phase::DONE));
}

TEST_F(StatusLineTest, testSetTotalMutantsDoesNotCrash) {
  StatusLine sl;
  EXPECT_NO_THROW(sl.setTotalMutants(0));
  EXPECT_NO_THROW(sl.setTotalMutants(100));
}

TEST_F(StatusLineTest, testSetMutantInfoDoesNotCrash) {
  StatusLine sl;
  EXPECT_NO_THROW(sl.setMutantInfo(1, "AOR", std::filesystem::path("foo.cpp"), 42));
}

TEST_F(StatusLineTest, testRecordResultDoesNotCrash) {
  StatusLine sl;
  for (int state = 0; state <= 4; ++state) {
    EXPECT_NO_THROW(sl.recordResult(state));
  }
}

TEST_F(StatusLineTest, testRecordResultInvalidStateDoesNotCrash) {
  StatusLine sl;
  EXPECT_NO_THROW(sl.recordResult(-1));
  EXPECT_NO_THROW(sl.recordResult(99));
}

TEST_F(StatusLineTest, testSetDryRunDoesNotCrash) {
  StatusLine sl;
  EXPECT_NO_THROW(sl.setDryRun(true));
  EXPECT_NO_THROW(sl.setDryRun(false));
}

TEST_F(StatusLineTest, testDestructorCallsDisable) {
  // If destructor crashes, the test process crashes too — RAII safety check
  EXPECT_NO_THROW({
    StatusLine sl;
    sl.enable();
    // sl destructor called at end of scope
  });
}

}  // namespace sentinel
