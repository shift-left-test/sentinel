/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <iostream>
#include <string>
#include "sentinel/Console.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Timestamper.hpp"

namespace sentinel {

namespace fs = std::filesystem;

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
  mTimestamper.reset();
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
  Console::print("\0337\033[{};1H\033[2K", mTermRows);
  clearScrollRegion();
  Console::print("\0338");
  Console::flush();
  mEnabled = false;
}

void StatusLine::setPhase(Phase phase) {
  mPhase = phase;
  redraw();
}

void StatusLine::setTotalMutants(size_t total) {
  mTotal = total;
  mProgressInterval = std::max(static_cast<size_t>(10), total / 10);
  redraw();
}

void StatusLine::setMutantInfo(size_t current, const std::string& op, const std::filesystem::path& file, size_t line) {
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
  if (mEnabled) {
    redraw();
  } else {
    logProgress();
  }
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
  Console::print("\033[2J\033[H");
  Console::flush();
  // Reserve last row for status line by setting scroll region to rows 1 through R-1
  Console::print("\033[1;{}r", mTermRows - 1);
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
  size_t denominator = mKilled + mSurvived + mTimeout + mRuntimeError;
  std::string scoreStr = denominator > 0
      ? fmt::format("{:.1f}%", 100.0 * static_cast<double>(mKilled) / static_cast<double>(denominator))
      : "N/A";
  return fmt::format("K:{} / S:{} / B:{} / T:{} / R:{} | Score: {} | Elapsed: {}",
                     mKilled, mSurvived, mBuildFail, mTimeout, mRuntimeError,
                     scoreStr, mTimestamper.toString(Timestamper::Format::Clock));
}

std::string StatusLine::buildProgressString(size_t current) const {
  return fmt::format("[{}/{}] ({}%) | {}", current, mTotal, mTotal > 0 ? current * 100 / mTotal : 0,
                     buildSummaryString());
}

std::string StatusLine::buildStatusString() const {
  std::string result;

  if (mDryRun) {
    result += " [DRY-RUN]";
  }

  result += fmt::format(" Phase: {:<12}", phaseLabel());

  if (mPhase == Phase::EVALUATION && !mOp.empty()) {
    result += fmt::format(" | {} {}:{}", mOp, mFile.string(), mLine);
  }

  if (mTotal > 0) {
    result += " " + buildProgressString(mCurrent);
  } else {
    result += " | " + buildSummaryString();
  }

  result += " ";
  return result;
}

void StatusLine::logProgress() {
  size_t processed = mKilled + mSurvived + mBuildFail + mTimeout + mRuntimeError;
  if (mTotal == 0 || mProgressInterval == 0) {
    return;
  }
  if (processed % mProgressInterval != 0 && processed != mTotal) {
    return;
  }
  Logger::info("Progress: {}", buildProgressString(processed));
}

}  // namespace sentinel
