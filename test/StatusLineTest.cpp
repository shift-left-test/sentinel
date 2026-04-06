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

using ::testing::HasSubstr;
using ::testing::Not;

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

TEST_F(StatusLineTest, testEvaluationPhaseShowsProgress) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::EVALUATION);
  sl.setTotalMutants(100);
  sl.setMutantInfo(50);
  for (int i = 0; i < 40; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 10; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("[50/100]"));
  EXPECT_THAT(text, HasSubstr("50%"));
  EXPECT_THAT(text, HasSubstr("\xe2\x9c\x97"));  // CrossMark
}

TEST_F(StatusLineTest, testEvaluationPhaseShowsSpinner) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::EVALUATION);
  sl.setTotalMutants(100);
  sl.setMutantInfo(1);

  std::string text = sl.getStatusText();

  // Spinner charset 15 frame 1 is U+2819, encoded as \xe2\xa0\x99
  EXPECT_THAT(text, HasSubstr("\xe2\xa0\x99"));
}

TEST_F(StatusLineTest, testNonEvaluationPhaseHasNoSpinner) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::BUILD_ORIG);

  std::string text = sl.getStatusText();

  // Charset 15 braille frames should not appear
  EXPECT_THAT(text, Not(HasSubstr("\xe2\xa0\x8b")));
  EXPECT_THAT(text, Not(HasSubstr("\xe2\xa0\x99")));
}

TEST_F(StatusLineTest, testEvaluationPhaseShowsGauge) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::EVALUATION);
  sl.setTotalMutants(100);
  sl.setMutantInfo(50);

  std::string text = sl.getStatusText();

  // gauge(0.5) renders full-block chars (U+2588 = \xe2\x96\x88)
  EXPECT_THAT(text, HasSubstr("\xe2\x96\x88"));
}

TEST_F(StatusLineTest, testNonEvaluationPhaseHasNoGauge) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::BUILD_ORIG);

  std::string text = sl.getStatusText();

  // No full-block character (U+2588) should appear without gauge
  EXPECT_THAT(text, Not(HasSubstr("\xe2\x96\x88")));
}

TEST_F(StatusLineTest, testNonEvaluationPhaseLayout) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::BUILD_ORIG);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("BUILD-ORIG"));
  EXPECT_THAT(text, HasSubstr("[0/0]"));
  EXPECT_THAT(text, HasSubstr("0%"));
}

TEST_F(StatusLineTest, testDryRunPrefix) {
  StatusLine sl;
  sl.setDryRun(true);
  sl.setPhase(StatusLine::Phase::EVALUATION);
  sl.setTotalMutants(100);
  sl.setMutantInfo(50);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("[DRY-RUN]"));
  EXPECT_THAT(text, HasSubstr("EVALUATION"));
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
  EXPECT_THAT(text, HasSubstr("66.7%"));
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

  EXPECT_THAT(output, Not(HasSubstr("Progress:")));
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

  EXPECT_THAT(output, Not(HasSubstr("Progress:")));
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
    EXPECT_THAT(sl.getStatusText(), HasSubstr(label)) << "Missing label for phase: " << label;
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

  EXPECT_THAT(text, HasSubstr("0.0%"));
}

TEST_F(StatusLineTest, testBuildProgressStringFormat) {
  StatusLine sl;
  sl.setTotalMutants(50);
  sl.setMutantInfo(25);
  for (int i = 0; i < 20; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 5; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));

  std::string text = sl.getStatusText();

  // mCurrent == 25 (set by setMutantInfo), mTotal == 50 → [25/50]
  EXPECT_THAT(text, HasSubstr("[25/50]"));
  // pct = 25 * 100 / 50 = 50 → "50%"
  EXPECT_THAT(text, HasSubstr("50%"));
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

TEST_F(StatusLineTest, testHundredPercentScore) {
  StatusLine sl;
  sl.setTotalMutants(5);
  for (int i = 0; i < 5; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("100.0%"));
}

TEST_F(StatusLineTest, testAllSurvivedScore) {
  StatusLine sl;
  sl.setTotalMutants(3);
  for (int i = 0; i < 3; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("0.0%"));
}

TEST_F(StatusLineTest, testGenerationPhaseLayout) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::GENERATION);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("GENERATION"));
  // Non-evaluation phase should not have braille spinner chars
  EXPECT_THAT(text, Not(HasSubstr("\xe2\xa0\x8b")));
  EXPECT_THAT(text, Not(HasSubstr("\xe2\xa0\x99")));
}

TEST_F(StatusLineTest, testDonePhaseLayout) {
  StatusLine sl;
  sl.setPhase(StatusLine::Phase::DONE);
  sl.setTotalMutants(10);
  for (int i = 0; i < 7; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 3; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("DONE"));
  EXPECT_THAT(text, HasSubstr("70.0%"));
}

TEST_F(StatusLineTest, testCountWidthWithLargeTotal) {
  StatusLine sl;
  sl.setTotalMutants(10000);
  sl.setMutantInfo(1);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("[1/10000]"));
}

TEST_F(StatusLineTest, testProgressWithZeroTotal) {
  StatusLine sl;
  sl.setTotalMutants(0);
  sl.setMutantInfo(0);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("[0/0]"));
  EXPECT_THAT(text, HasSubstr("0%"));
}

TEST_F(StatusLineTest, testDryRunWithNonEvaluationPhase) {
  StatusLine sl;
  sl.setDryRun(true);
  sl.setPhase(StatusLine::Phase::GENERATION);

  std::string text = sl.getStatusText();

  EXPECT_THAT(text, HasSubstr("[DRY-RUN]"));
  EXPECT_THAT(text, HasSubstr("GENERATION"));
}

TEST_F(StatusLineTest, testMixedResultsCorrectCounts) {
  StatusLine sl;
  sl.setTotalMutants(8);
  for (int i = 0; i < 3; ++i) sl.recordResult(static_cast<int>(MutationState::KILLED));
  for (int i = 0; i < 2; ++i) sl.recordResult(static_cast<int>(MutationState::SURVIVED));
  sl.recordResult(static_cast<int>(MutationState::BUILD_FAILURE));
  sl.recordResult(static_cast<int>(MutationState::TIMEOUT));
  sl.recordResult(static_cast<int>(MutationState::RUNTIME_ERROR));

  std::string text = sl.getStatusText();

  // score = 3 / (3 + 2) = 60.0%; abnormals excluded from denominator
  EXPECT_THAT(text, HasSubstr("60.0%"));
}

}  // namespace sentinel
