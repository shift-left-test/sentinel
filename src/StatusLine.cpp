/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <iostream>
#include <string>
#include "sentinel/Console.hpp"
#include "sentinel/util/Utf8Char.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/util/signal.hpp"

namespace sentinel {

namespace {
StatusLine* sActiveInstance = nullptr;
struct sigaction sPrevTstp {};
struct sigaction sPrevCont {};
}  // namespace

namespace detail {
void handleSigtstp(int /*signum*/) {
  if (sActiveInstance != nullptr && sActiveInstance->isEnabled()) {
    sActiveInstance->suspend();
  }
  signal::setSignalHandler(SIGTSTP, SIG_DFL);
  raise(SIGTSTP);
}

void handleSigcont(int /*signum*/) {
  if (sActiveInstance != nullptr) {
    signal::setSignalHandler(SIGTSTP, handleSigtstp);
    sActiveInstance->resume();
  }
}
}  // namespace detail

StatusLine::StatusLine() = default;

std::string StatusLine::getStatusText() const {
  return buildStatusString();
}

StatusLine::~StatusLine() {
  if (mEnabled) {
    disable();
  }
}

void StatusLine::enable() {
  if (!isatty(STDOUT_FILENO)) {
    return;
  }
  queryTermSize();
  mTimestamper.reset();
  mEnabled = true;
  installSuspendHandlers();
  setScrollRegion();
  redraw();
}

void StatusLine::disable() {
  if (!mEnabled) {
    return;
  }
  // Save cursor → move to status row → clear it → reset scroll region (DECSTBM homes
  // the cursor, so it must come before ESC 8) → restore original cursor position.
  Console::print("\0337\033[{};1H\033[2K", mTermRows);
  clearScrollRegion();
  Console::print("\0338");
  Console::flush();
  uninstallSuspendHandlers();
  mEnabled = false;
}

void StatusLine::setPhase(Phase phase) {
  mPhase = phase;
  redraw();
}

void StatusLine::setTotalMutants(size_t total) {
  mTotal = total;
  redraw();
}

void StatusLine::setMutantInfo(size_t current) {
  mCurrent = current;
  redraw();
}

void StatusLine::setDryRun(bool dryRun) {
  mDryRun = dryRun;
  redraw();
}

void StatusLine::recordResult(int state) {
  switch (static_cast<MutationState>(state)) {
    case MutationState::KILLED:
      mKilled++;
      break;
    case MutationState::SURVIVED:
      mSurvived++;
      break;
    case MutationState::RUNTIME_ERROR:
    case MutationState::BUILD_FAILURE:
    case MutationState::TIMEOUT:
      mAbnormal++;
      break;
  }
  if (mEnabled) {
    redraw();
  }
}

void StatusLine::queryTermSize() {
  struct winsize ws = {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
    mTermRows = ws.ws_row;
    mTermCols = ws.ws_col;
  }
}

void StatusLine::installSuspendHandlers() {
  sActiveInstance = this;
  signal::getSigaction(SIGTSTP, &sPrevTstp);
  signal::getSigaction(SIGCONT, &sPrevCont);
  signal::setSignalHandler(SIGTSTP, detail::handleSigtstp);
  signal::setSignalHandler(SIGCONT, detail::handleSigcont);
}

void StatusLine::uninstallSuspendHandlers() {
  signal::setSigaction(SIGTSTP, &sPrevTstp);
  signal::setSigaction(SIGCONT, &sPrevCont);
  sActiveInstance = nullptr;
}

void StatusLine::suspend() {
  if (!mEnabled) {
    return;
  }
  Console::print("\0337\033[{};1H\033[2K", mTermRows);
  clearScrollRegion();
  Console::print("\0338");
  Console::flush();
  mEnabled = false;
}

void StatusLine::resume() {
  if (!isatty(STDOUT_FILENO)) {
    return;
  }
  queryTermSize();
  mEnabled = true;
  setScrollRegion();
  redraw();
}

void StatusLine::setScrollRegion() {
  if (!mEnabled) {
    return;
  }
  // Reserve last row for status line by setting scroll region to rows 1 through R-1.
  // \n ensures there is a blank row for the status line (and scrolls content up if the
  // cursor is already at the bottom). \033[1A moves the cursor back up one row so it
  // stays within the scroll region. Saving and restoring around DECSTBM then keeps
  // subsequent output flowing naturally from where it was — matching --no-status-line UX.
  Console::print("\n\033[1A\0337\033[1;{}r\0338", mTermRows - 1);
  Console::flush();
}

void StatusLine::clearScrollRegion() {
  // Reset scroll region to the full terminal (DECSTBM with no args)
  Console::print("\033[r");
}

void StatusLine::redraw() {
  if (!mEnabled) {
    return;
  }
  std::string status = buildStatusString();
  if (static_cast<int>(status.size()) > mTermCols) {
    status.resize(mTermCols);
  }
  // Save cursor, jump to status row, clear line, print reverse-video content, restore cursor
  Console::print("\0337\033[{};1H\033[2K", mTermRows);
  Console::print("\033[7m{}\033[0m\0338", status);
  Console::flush();
}

std::string StatusLine::phaseLabel() const {
  switch (mPhase) {
    case Phase::INIT:
      return "INIT";
    case Phase::BUILD_ORIG:
      return "BUILD-ORIG";
    case Phase::TEST_ORIG:
      return "TEST-ORIG";
    case Phase::GENERATION:
      return "GENERATION";
    case Phase::EVALUATION:
      return "EVALUATION";
    case Phase::REPORT:
      return "REPORT";
    case Phase::DONE:
      return "DONE";
  }
  return "UNKNOWN";
}

std::string StatusLine::buildSummaryString() const {
  size_t denominator = mKilled + mSurvived;
  double score = denominator > 0
      ? 100.0 * static_cast<double>(mKilled) / static_cast<double>(denominator)
      : 0.0;
  std::string scoreStr = fmt::format("{:.1f}%", score);
  int w = countWidth();
  return fmt::format("{}{:>{}} {}{:>{}} {}{:>{}}  {}  {:>6}  {}  {}",
                     Utf8Char::CrossMark, mKilled, w,
                     Utf8Char::CheckMark, mSurvived, w,
                     Utf8Char::Warning, mAbnormal, w,
                     Utf8Char::VerticalBar, scoreStr,
                     Utf8Char::VerticalBar,
                     mTimestamper.toString(Timestamper::Format::Clock));
}

int StatusLine::countWidth() const {
  return std::max(2, static_cast<int>(fmt::formatted_size("{}", mTotal)));
}

std::string StatusLine::buildProgressString(size_t current) const {
  size_t pct = mTotal > 0 ? current * 100 / mTotal : 0;
  return fmt::format("[{}/{}] ({}%)  {}  {}", current, mTotal, pct,
                     Utf8Char::VerticalBar, buildSummaryString());
}

std::string StatusLine::buildStatusString() const {
  std::string result;

  if (mDryRun) {
    result += " [DRY-RUN]";
  }

  result += fmt::format(" {:<10}", phaseLabel());

  result += fmt::format("  {}  ", Utf8Char::VerticalBar) + buildProgressString(mCurrent);

  return result;
}

void StatusLine::logSummary() const {
  if (mTotal == 0) {
    return;
  }
  size_t processed = mKilled + mSurvived + mAbnormal;
  Logger::info("Summary: {}", buildProgressString(processed));
}

}  // namespace sentinel
