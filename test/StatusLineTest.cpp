/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <csignal>
#include <string>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/MutationState.hpp"
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
  EXPECT_NO_THROW(sl.setMutantInfo(1));
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

TEST_F(StatusLineTest, testSummaryUsesUtf8Icons) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(10);
  for (int i = 0; i < 6; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 2; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));
  sl.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));
  sl.recordResult(static_cast<int>(MutationState::TIMEOUT));

  testing::internal::CaptureStderr();
  sl.logSummary();
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, testing::HasSubstr("\xe2\x9c\x97"));
  EXPECT_THAT(output, testing::HasSubstr("\xe2\x9c\x93"));
  EXPECT_THAT(output, testing::HasSubstr("\xe2\x9a\xa0"));
  EXPECT_THAT(output, testing::Not(testing::HasSubstr("K:")));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testScoreExcludesAbnormalFromDenominator) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(10);
  // Record: 6 killed, 2 survived, 1 build failure, 1 timeout
  for (int i = 0; i < 6; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 2; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));
  sl.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));
  sl.recordResult(static_cast<int>(MutationState::TIMEOUT));

  testing::internal::CaptureStderr();
  sl.logSummary();
  std::string output = testing::internal::GetCapturedStderr();

  // Score should be 6/(6+2) = 75.0%, NOT 6/(6+2+2) = 60.0%
  EXPECT_THAT(output, testing::HasSubstr("75.0%"));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testLogSummaryWithPartialResults) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(10);
  for (int i = 0; i < 3; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  sl.recordResult(static_cast<int>(MutationState::SURVIVED));
  // 4 processed out of 10

  testing::internal::CaptureStderr();
  sl.logSummary();
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, testing::HasSubstr("Summary:"));
  EXPECT_THAT(output, testing::HasSubstr("[4/10]"));
  EXPECT_THAT(output, testing::HasSubstr("40%"));
  // score = 3/(3+1) = 75.0%
  EXPECT_THAT(output, testing::HasSubstr("75.0%"));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testEvaluationPhaseShowsProgress) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::EVALUATION);
  sl.setTotalMutants(100);
  sl.setMutantInfo(50);
  for (int i = 0; i < 40; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 10; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, testing::HasSubstr("[50/100]"));
  EXPECT_THAT(text, testing::HasSubstr("50%"));
  EXPECT_THAT(text, testing::HasSubstr("\xe2\x9c\x97"));  // CrossMark
}

TEST_F(StatusLineTest, testNonEvaluationPhaseLayout) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::BUILD_ORIG);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, testing::HasSubstr("BUILD-ORIG"));
  EXPECT_THAT(text, testing::HasSubstr("[0/0]"));
  EXPECT_THAT(text, testing::HasSubstr("0%"));
}

TEST_F(StatusLineTest, testAdaptiveCountWidth) {
  Logger::setLevel(Logger::Level::INFO);

  StatusLine sl1;
  sl1.setTotalMutants(10);
  for (int i = 0; i < 8; ++i) sl1.recordResult(static_cast<int>(MutationState::KILLED));
  sl1.recordResult(static_cast<int>(MutationState::SURVIVED));
  sl1.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));

  testing::internal::CaptureStderr();
  sl1.logSummary();
  std::string output1 = testing::internal::GetCapturedStderr();
  EXPECT_THAT(output1, testing::HasSubstr("[10/10]"));

  StatusLine sl2;
  sl2.setTotalMutants(10000);
  for (int i = 0; i < 8000; ++i) sl2.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 2000; ++i) sl2.recordResult(static_cast<int>(MutationState::SURVIVED));

  testing::internal::CaptureStderr();
  sl2.logSummary();
  std::string output2 = testing::internal::GetCapturedStderr();
  EXPECT_THAT(output2, testing::HasSubstr("[10000/10000]"));

  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testDryRunPrefix) {
  StatusLine sl;
  sl.setDryRun(true);
  sl.setPhase(StatusLine::Phase::EVALUATION);
  sl.setTotalMutants(100);
  sl.setMutantInfo(50);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, testing::HasSubstr("[DRY-RUN]"));
  EXPECT_THAT(text, testing::HasSubstr("EVALUATION"));
}

TEST_F(StatusLineTest, testRecordResultCountersCorrect) {
  StatusLine sl;
  sl.setTotalMutants(100);
  sl.recordResult(static_cast<int>(MutationState::KILLED));
  sl.recordResult(static_cast<int>(MutationState::KILLED));
  sl.recordResult(static_cast<int>(MutationState::SURVIVED));
  sl.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));
  sl.recordResult(static_cast<int>(MutationState::TIMEOUT));
  sl.recordResult(static_cast<int>(MutationState::RUNTIME_ERROR));

  std::string text = sl.getStatusText();

  // score = 2 / (2 + 1) = 66.7%; abnormals excluded from denominator
  EXPECT_THAT(text, testing::HasSubstr("66.7%"));
}

TEST_F(StatusLineTest, testRecordResultDoesNotLogProgress) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(100);

  for (int i = 0; i < 9; ++i) {
    sl.recordResult(static_cast<int>(MutationState::KILLED));
  }

  testing::internal::CaptureStderr();
  sl.recordResult(static_cast<int>(MutationState::KILLED));
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, testing::Not(testing::HasSubstr("Progress:")));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testRecordResultDoesNotLogAtCompletion) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(3);

  sl.recordResult(static_cast<int>(MutationState::KILLED));
  sl.recordResult(static_cast<int>(MutationState::KILLED));

  testing::internal::CaptureStderr();
  sl.recordResult(static_cast<int>(MutationState::KILLED));
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, testing::Not(testing::HasSubstr("Progress:")));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testLogSummaryOutputsProgress) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(10);
  for (int i = 0; i < 6; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 2; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));
  sl.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));
  sl.recordResult(static_cast<int>(MutationState::TIMEOUT));

  testing::internal::CaptureStderr();
  sl.logSummary();
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, testing::HasSubstr("Summary:"));
  EXPECT_THAT(output, testing::HasSubstr("[10/10]"));
  EXPECT_THAT(output, testing::HasSubstr("100%"));
  EXPECT_THAT(output, testing::HasSubstr("75.0%"));
  EXPECT_THAT(output, testing::HasSubstr("\xe2\x9c\x97"));
  EXPECT_THAT(output, testing::HasSubstr("\xe2\x9c\x93"));
  EXPECT_THAT(output, testing::HasSubstr("\xe2\x9a\xa0"));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testLogSummarySkippedForZeroTotal) {
  Logger::setLevel(Logger::Level::INFO);
  StatusLine sl;
  sl.setTotalMutants(0);

  testing::internal::CaptureStderr();
  sl.logSummary();
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, testing::Not(testing::HasSubstr("Summary:")));
  Logger::setLevel(Logger::Level::OFF);
}

TEST_F(StatusLineTest, testPhaseLabelCoversAllPhases) {
  const std::vector<std::pair<StatusLine::Phase, std::string>> phases = {
      {StatusLine::Phase::INIT, "INIT"},
      {StatusLine::Phase::BUILD_ORIG, "BUILD-ORIG"},
      {StatusLine::Phase::TEST_ORIG, "TEST-ORIG"},
      {StatusLine::Phase::GENERATION, "GENERATION"},
      {StatusLine::Phase::EVALUATION, "EVALUATION"},
      {StatusLine::Phase::REPORT, "REPORT"},
      {StatusLine::Phase::DONE, "DONE"},
  };

  for (const auto& [phase, label] : phases) {
    StatusLine sl;
    sl.setPhase(phase);
    EXPECT_THAT(sl.getStatusText(), testing::HasSubstr(label)) << "Missing label for phase: " << label;
  }
}

TEST_F(StatusLineTest, testZeroDenominatorScoreIsZero) {
  StatusLine sl;
  sl.setTotalMutants(3);
  // Record only abnormal results — denominator (killed+survived) stays 0
  sl.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));
  sl.recordResult(static_cast<int>(MutationState::TIMEOUT));
  sl.recordResult(static_cast<int>(MutationState::RUNTIME_ERROR));

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, testing::HasSubstr("0.0%"));
}

TEST_F(StatusLineTest, testBuildProgressStringFormat) {
  StatusLine sl;
  sl.setTotalMutants(50);
  sl.setMutantInfo(25);
  for (int i = 0; i < 20; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 5; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));

  std::string text = sl.getStatusText();

  // mCurrent == 25 (set by setMutantInfo), mTotal == 50 → [25/50]
  EXPECT_THAT(text, testing::HasSubstr("[25/50]"));
  // pct = 25 * 100 / 50 = 50 → "50%"
  EXPECT_THAT(text, testing::HasSubstr("50%"));
}


TEST_F(StatusLineTest, testRaiseSigtstpWithIgnoreDoesNotCrash) {
  struct sigaction prev {};
  struct sigaction ignore {};
  ignore.sa_handler = SIG_IGN;
  sigemptyset(&ignore.sa_mask);
  sigaction(SIGTSTP, &ignore, &prev);

  StatusLine sl;
  EXPECT_NO_THROW({
    raise(SIGTSTP);
  });

  sigaction(SIGTSTP, &prev, nullptr);
}

TEST_F(StatusLineTest, testDisableRestoresSuspendHandlers) {
  StatusLine sl;
  struct sigaction before {};
  sigaction(SIGTSTP, nullptr, &before);

  sl.enable();
  sl.disable();

  struct sigaction after {};
  sigaction(SIGTSTP, nullptr, &after);
  EXPECT_EQ(before.sa_handler, after.sa_handler);
}

}  // namespace sentinel
