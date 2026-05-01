/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STATUSLINE_HPP_
#define INCLUDE_SENTINEL_STATUSLINE_HPP_

#include <csignal>
#include <string>
#include <ftxui/dom/elements.hpp>
#include "sentinel/MutationState.hpp"
#include "sentinel/Timestamper.hpp"

namespace sentinel {

namespace detail {
void handleSigtstp(int signum);
void handleSigcont(int signum);
void handleSigwinch(int signum);
}  // namespace detail

/**
 * @brief Terminal status line displayed at the bottom of the terminal during sentinel run.
 *
 * Uses ANSI scroll region (DECSTBM) to reserve the last terminal row for status display,
 * so normal log output scrolls above it without disturbing the status line.
 * Automatically disabled when stdout is not a TTY (pipe/redirect).
 */
class StatusLine {
  friend void detail::handleSigtstp(int);
  friend void detail::handleSigcont(int);
  friend void detail::handleSigwinch(int);

 public:
  /**
   * @brief Execution phase shown in the status line.
   */
  enum class Phase { INIT, BUILD_ORIG, TEST_ORIG, GENERATION, EVALUATION, REPORT, DONE };

  /**
   * @brief Default constructor.
   */
  StatusLine();
  StatusLine(const StatusLine&) = delete;
  StatusLine& operator=(const StatusLine&) = delete;

  /**
   * @brief Destructor. Calls disable() if the status line is still active.
   */
  ~StatusLine();

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
   * @brief Set the total number of progress steps (units of work) and redraw.
   *
   * Used by stages that report incremental progress. The unit varies by
   * phase (files during GENERATION, mutants during EVALUATION); the
   * StatusLine itself is unit-agnostic.
   *
   * @param total Total number of progress steps.
   */
  void setProgressTotal(std::size_t total);

  /**
   * @brief Update the currently-completed progress step count and redraw.
   *
   * @param current 1-based count of completed steps.
   */
  void setProgressCurrent(std::size_t current);

  /**
   * @brief Record the result of the last mutant and redraw.
   *
   * @param state    MutationState of the result.
   * @param uncovered true if the mutant was lcov-skipped (subset of SURVIVED).
   *                  Counted in mUncovered separately to display the SURVIVED breakdown.
   */
  void recordResult(MutationState state, bool uncovered);

  /**
   * @brief Set dry-run mode indicator and redraw.
   *
   * @param dryRun True if running in dry-run mode.
   */
  void setDryRun(bool dryRun);

  /**
   * @brief Check whether the status line is currently active.
   *
   * @return true if the status line is enabled and drawing to the terminal.
   */
  bool isEnabled() const { return mEnabled; }

  /**
   * @brief Return the current status string (for testing).
   *
   * @return the formatted status line content.
   */
  std::string getStatusText() const;

 private:
  void openTty();
  void closeTty();
  void activate();
  void deactivate();
  void refreshTermSize();
  void setScrollRegion();
  void clearScrollRegion();
  void redraw();
  ftxui::Element buildElement() const;
  ftxui::Element buildSummaryElement() const;
  ftxui::Element buildProgressElement(std::size_t current) const;
  std::string renderToString(const ftxui::Element& element) const;
  std::string phaseLabel() const;
  int countWidth() const;
  void installSignalHandlers();
  void uninstallSignalHandlers();
  void suspend();
  void resume();
  void handleResize();
  /** @brief Write raw data to the TTY file descriptor, retrying on EINTR. */
  void writeTty(const std::string& data);

  bool mEnabled = false;
  bool mDryRun = false;
  volatile sig_atomic_t mResized = 0;
  int mTtyFd = -1;
  static constexpr int kDefaultTermRows = 24;
  static constexpr int kDefaultTermCols = 120;
  int mTermRows = kDefaultTermRows;
  int mTermCols = kDefaultTermCols;
  Phase mPhase = Phase::INIT;
  std::size_t mCurrent = 0;
  std::size_t mTotal = 0;
  std::size_t mKilled = 0;
  std::size_t mSurvived = 0;
  std::size_t mAbnormal = 0;  ///< Combined BUILD_FAILURE + TIMEOUT + RUNTIME_ERROR count.
  std::size_t mUncovered = 0;  ///< Subset of mSurvived that were lcov-skipped.
  Timestamper mTimestamper;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STATUSLINE_HPP_
