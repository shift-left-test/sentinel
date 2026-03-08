/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include "sentinel/StatusLine.hpp"

#include <fmt/core.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>

#include "sentinel/MutationState.hpp"

namespace sentinel {

StatusLine::StatusLine() = default;

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
  mStartTime = std::chrono::steady_clock::now();
  mEnabled = true;
  setScrollRegion();
  redraw();
}

void StatusLine::disable() {
  if (!mEnabled) {
    return;
  }
  // Save cursor → move to status row → clear it → reset scroll region (DECSTBM homes
  // the cursor, so it must come before ESC 8) → restore original cursor position.
  std::cout << "\0337" << fmt::format("\033[{};1H", mTermRows) << "\033[2K";
  clearScrollRegion();
  std::cout << "\0338";
  std::cout.flush();
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

void StatusLine::setMutantInfo(size_t current, const std::string& op, const std::string& file, size_t line) {
  mCurrent = current;
  mOp = op;
  mFile = file;
  mLine = line;
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
      mRuntimeError++;
      break;
    case MutationState::BUILD_FAILURE:
      mBuildFail++;
      break;
    case MutationState::TIMEOUT:
      mTimeout++;
      break;
  }
  redraw();
}

void StatusLine::queryTermSize() {
  struct winsize ws = {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
    mTermRows = ws.ws_row;
    mTermCols = ws.ws_col;
  }
}

void StatusLine::setScrollRegion() {
  if (!mEnabled) {
    return;
  }
  // Clear screen
  std::cout << "\033[2J\033[H" << std::flush;
  // Reserve last row for status line by setting scroll region to rows 1 through R-1
  std::cout << fmt::format("\033[1;{}r", mTermRows - 1);
  std::cout.flush();
}

void StatusLine::clearScrollRegion() {
  // Reset scroll region to the full terminal (DECSTBM with no args)
  std::cout << "\033[r";
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
  std::cout << "\0337" << fmt::format("\033[{};1H", mTermRows) << "\033[2K"
            << "\033[7m" << status << "\033[0m"
            << "\0338";
  std::cout.flush();
}

std::string StatusLine::phaseLabel() const {
  switch (mPhase) {
    case Phase::INIT:
      return "INIT";
    case Phase::BUILD_ORIG:
      return "BUILD-ORIG";
    case Phase::TEST_ORIG:
      return "TEST-ORIG";
    case Phase::POPULATE:
      return "POPULATE";
    case Phase::MUTANT:
      return "MUTANT";
    case Phase::REPORT:
      return "REPORT";
    case Phase::DONE:
      return "DONE";
  }
  return "UNKNOWN";
}

std::string StatusLine::getElapsedStr() const {
  auto now = std::chrono::steady_clock::now();
  auto secs = std::chrono::duration_cast<std::chrono::seconds>(now - mStartTime).count();
  int h = static_cast<int>(secs / 3600);
  int m = static_cast<int>((secs % 3600) / 60);
  int s = static_cast<int>(secs % 60);
  return fmt::format("{:02d}:{:02d}:{:02d}", h, m, s);
}

std::string StatusLine::buildStatusString() const {
  std::string result;

  if (mDryRun) {
    result += " [DRY-RUN]";
  }

  // Phase label, padded to 10 chars for alignment across all phase names
  result += fmt::format(" Phase: {:<10}", phaseLabel());

  if (mTotal > 0) {
    result += fmt::format(" [{}/{}]", mCurrent, mTotal);
  }

  if (mPhase == Phase::MUTANT && !mOp.empty()) {
    result += fmt::format(" | {} {}:{}", mOp, mFile, mLine);
  }

  result += fmt::format(" | K:{} / S:{} / B:{} / T:{} / R:{}", mKilled, mSurvived, mBuildFail, mTimeout, mRuntimeError);

  size_t denominator = mKilled + mSurvived + mTimeout + mRuntimeError;
  if (denominator > 0) {
    double score = (100.0 * static_cast<double>(mKilled)) / static_cast<double>(denominator);
    result += fmt::format(" | Score: {:.1f}%", score);
  } else {
    result += " | Score: N/A";
  }

  result += " | Elapsed: " + getElapsedStr();
  return result;
}

}  // namespace sentinel
