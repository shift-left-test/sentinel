/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STATUSLINE_HPP_
#define INCLUDE_SENTINEL_STATUSLINE_HPP_

#include <string>
#include "sentinel/TimeStamper.hpp"

namespace sentinel {

/**
 * @brief Terminal status line displayed at the bottom of the terminal during sentinel run.
 *
 * Uses ANSI scroll region (DECSTBM) to reserve the last terminal row for status display,
 * so normal log output scrolls above it without disturbing the status line.
 * Automatically disabled when stdout is not a TTY (pipe/redirect).
 */
class StatusLine {
 public:
  /**
   * @brief Execution phase shown in the status line.
   */
  enum class Phase { INIT, BUILD_ORIG, TEST_ORIG, POPULATE, MUTANT, REPORT, DONE };

  /**
   * @brief Default constructor.
   */
  StatusLine();

  /**
   * @brief Destructor. Calls disable() if the status line is still active.
   */
  ~StatusLine();

  StatusLine(const StatusLine&) = delete;
  StatusLine& operator=(const StatusLine&) = delete;

  /**
   * @brief Activate the status line.
   *
   * Checks isatty(), queries the terminal size, sets the ANSI scroll region,
   * and draws the initial status line. No-op if stdout is not a TTY.
   */
  void enable();

  /**
   * @brief Deactivate the status line and restore the terminal to its original state.
   *
   * Clears the status row, resets the scroll region, and restores the cursor.
   * No-op if already disabled.
   */
  void disable();

  /**
   * @brief Update the current execution phase and redraw.
   *
   * @param phase New execution phase.
   */
  void setPhase(Phase phase);

  /**
   * @brief Set the total mutant count and redraw.
   *
   * @param total Total number of mutants to process.
   */
  void setTotalMutants(size_t total);

  /**
   * @brief Update the currently-processed mutant information and redraw.
   *
   * @param current 1-based index of the mutant being processed.
   * @param op      Mutation operator name (e.g. "AOR").
   * @param file    Source filename (basename only).
   * @param line    Source line number of the mutation.
   */
  void setMutantInfo(size_t current, const std::string& op, const std::string& file, size_t line);

  /**
   * @brief Record the result of the last mutant and redraw.
   *
   * @param state MutationState cast to int (KILLED=0, SURVIVED=1, RUNTIME_ERROR=2,
   *              BUILD_FAILURE=3, TIMEOUT=4).
   */
  void recordResult(int state);

  /**
   * @brief Set dry-run mode indicator and redraw.
   *
   * @param dryRun True if running in dry-run mode.
   */
  void setDryRun(bool dryRun);

 private:
  void queryTermSize();
  void setScrollRegion();
  void clearScrollRegion();
  void redraw();
  std::string buildStatusString() const;
  std::string phaseLabel() const;

  bool mEnabled = false;
  bool mDryRun = false;
  int mTermRows = 24;
  int mTermCols = 80;
  Phase mPhase = Phase::INIT;
  size_t mCurrent = 0;
  size_t mTotal = 0;
  size_t mLine = 0;
  std::string mOp;
  std::string mFile;
  size_t mKilled = 0;
  size_t mSurvived = 0;
  size_t mBuildFail = 0;
  size_t mTimeout = 0;
  size_t mRuntimeError = 0;
  TimeStamper mTimeStamper;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STATUSLINE_HPP_
