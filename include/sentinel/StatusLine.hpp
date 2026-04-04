/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STATUSLINE_HPP_
#define INCLUDE_SENTINEL_STATUSLINE_HPP_

#include <string>
#include <ftxui/dom/elements.hpp>
#include "sentinel/Timestamper.hpp"

namespace sentinel {

namespace detail {
void handleSigtstp(int signum);
void handleSigcont(int signum);
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
   * @brief Set the total mutant count and redraw.
   *
   * @param total Total number of mutants to process.
   */
  void setTotalMutants(size_t total);

  /**
   * @brief Update the currently-processed mutant index and redraw.
   *
   * @param current 1-based index of the mutant being processed.
   */
  void setMutantInfo(size_t current);

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
  void refreshTermSize();
  void setScrollRegion();
  void clearScrollRegion();
  void redraw();
  ftxui::Element buildElement() const;
  ftxui::Element buildSummaryElement() const;
  ftxui::Element buildProgressElement(size_t current) const;
  std::string renderToString(const ftxui::Element& element) const;
  std::string phaseLabel() const;
  int countWidth() const;
  void installSuspendHandlers();
  void uninstallSuspendHandlers();
  void suspend();
  void resume();

  bool mEnabled = false;
  bool mDryRun = false;
  int mTermRows = 24;
  int mTermCols = 80;
  Phase mPhase = Phase::INIT;
  size_t mCurrent = 0;
  size_t mTotal = 0;
  size_t mKilled = 0;
  size_t mSurvived = 0;
  size_t mAbnormal = 0;  ///< Combined BUILD_FAILURE + TIMEOUT + RUNTIME_ERROR count.
  Timestamper mTimestamper;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STATUSLINE_HPP_
