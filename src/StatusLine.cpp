/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <iostream>
#include <string>
#include <utility>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
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
  return renderToString(buildElement());
}

StatusLine::~StatusLine() {
  if (mEnabled) {
    disable();
  }
}

void StatusLine::refreshTermSize() {
  auto dim = ftxui::Terminal::Size();
  mTermRows = dim.dimy;
  mTermCols = dim.dimx;
}

void StatusLine::enable() {
  if (!isatty(STDOUT_FILENO)) {
    return;
  }
  refreshTermSize();
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
  refreshTermSize();
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
  std::string rendered = renderToString(buildElement());
  // Save cursor, jump to status row, clear line, print rendered content, restore cursor
  Console::print("\0337\033[{};1H\033[2K{}\0338", mTermRows, rendered);
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

ftxui::Element StatusLine::buildSummaryElement() const {
  size_t denominator = mKilled + mSurvived;
  double score = denominator > 0
      ? 100.0 * static_cast<double>(mKilled) / static_cast<double>(denominator)
      : 0.0;
  std::string scoreStr = fmt::format("{:.1f}%", score);
  int w = countWidth();
  return ftxui::hbox({
      ftxui::text(fmt::format("{}{:>{}} ", Utf8Char::CrossMark, mKilled, w)),
      ftxui::text(fmt::format("{}{:>{}} ", Utf8Char::CheckMark, mSurvived, w)),
      ftxui::text(fmt::format("{}{:>{}} ", Utf8Char::Warning, mAbnormal, w)),
      ftxui::separatorLight(),
      ftxui::text(fmt::format(" {:>6} ", scoreStr)),
      ftxui::separatorLight(),
      ftxui::text(fmt::format(" {}", mTimestamper.toString(Timestamper::Format::Clock))),
  });
}

int StatusLine::countWidth() const {
  return std::max(2, static_cast<int>(fmt::formatted_size("{}", mTotal)));
}

ftxui::Element StatusLine::buildProgressElement(size_t current) const {
  size_t pct = mTotal > 0 ? current * 100 / mTotal : 0;
  return ftxui::hbox({
      ftxui::text(fmt::format("[{}/{}] ({}%) ", current, mTotal, pct)),
      ftxui::separatorLight(),
      ftxui::text(" "),
      buildSummaryElement(),
  });
}

ftxui::Element StatusLine::buildElement() const {
  ftxui::Elements elements;
  if (mDryRun) {
    elements.push_back(ftxui::text(" [DRY-RUN]"));
  }
  elements.push_back(ftxui::text(fmt::format(" {:<10}", phaseLabel())));
  elements.push_back(ftxui::text(" "));
  elements.push_back(ftxui::separatorLight());
  elements.push_back(ftxui::text(" "));
  elements.push_back(buildProgressElement(mCurrent));
  elements.push_back(ftxui::filler());
  return ftxui::hbox(std::move(elements)) | ftxui::inverted;
}

std::string StatusLine::renderToString(const ftxui::Element& element) const {
  auto screen = ftxui::Screen::Create(
      ftxui::Dimension::Fixed(mTermCols), ftxui::Dimension::Fixed(1));
  ftxui::Render(screen, element);
  return screen.ToString();
}

void StatusLine::logSummary() const {
  if (mTotal == 0) {
    return;
  }
  size_t processed = mKilled + mSurvived + mAbnormal;
  auto element = buildProgressElement(processed);
  auto dim = ftxui::Dimension::Fit(element);
  auto screen = ftxui::Screen::Create(dim);
  ftxui::Render(screen, element);
  Logger::info("Summary: {}", screen.ToString());
}

}  // namespace sentinel
