/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <fmt/core.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <csignal>
#include <string>
#include <utility>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include "sentinel/util/Utf8Char.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/util/signal.hpp"

namespace sentinel {

namespace {
constexpr int kSpinnerCharset = 15;
constexpr int kGaugeWidth = 20;
constexpr int kPhaseLabelWidth = 10;

StatusLine* sActiveInstance = nullptr;
struct sigaction sPrevTstp {};
struct sigaction sPrevCont {};
struct sigaction sPrevWinch {};
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

void handleSigwinch(int /*signum*/) {
  if (sActiveInstance != nullptr) {
    sActiveInstance->mResized = 1;
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

void StatusLine::openTty() {
  mTtyFd = open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
}

void StatusLine::closeTty() {
  if (mTtyFd >= 0) {
    close(mTtyFd);
    mTtyFd = -1;
  }
}

void StatusLine::writeTty(const std::string& data) {
  if (mTtyFd < 0) {
    return;
  }
  const char* ptr = data.data();
  size_t remaining = data.size();
  while (remaining > 0) {
    ssize_t written = write(mTtyFd, ptr, remaining);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    ptr += written;
    remaining -= static_cast<size_t>(written);
  }
}

void StatusLine::activate() {
  refreshTermSize();
  mEnabled = true;
  openTty();
  setScrollRegion();
  redraw();
}

void StatusLine::deactivate() {
  // Save cursor → move to status row → clear it → reset scroll region (DECSTBM homes
  // the cursor, so it must come before ESC 8) → restore original cursor position.
  writeTty(fmt::format("\0337\033[{};1H\033[2K", mTermRows));
  clearScrollRegion();
  writeTty("\0338");
  closeTty();
  mEnabled = false;
}

void StatusLine::enable() {
  if (!isatty(STDOUT_FILENO)) {
    return;
  }
  mTimestamper.reset();
  installSignalHandlers();
  activate();
}

void StatusLine::disable() {
  if (!mEnabled) {
    return;
  }
  deactivate();
  uninstallSignalHandlers();
}

void StatusLine::setPhase(Phase phase) {
  mPhase = phase;
  redraw();
}

void StatusLine::setProgressTotal(size_t total) {
  if (mTotal == total) {
    return;
  }
  mTotal = total;
  redraw();
}

void StatusLine::setProgressCurrent(size_t current) {
  if (mCurrent == current) {
    return;
  }
  mCurrent = current;
  redraw();
}

void StatusLine::setDryRun(bool dryRun) {
  mDryRun = dryRun;
  redraw();
}

void StatusLine::recordResult(MutationState state, bool uncovered) {
  switch (state) {
    case MutationState::KILLED:
      mKilled++;
      break;
    case MutationState::SURVIVED:
      mSurvived++;
      if (uncovered) {
        mUncovered++;
      }
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

void StatusLine::installSignalHandlers() {
  sActiveInstance = this;
  signal::getSigaction(SIGTSTP, &sPrevTstp);
  signal::getSigaction(SIGCONT, &sPrevCont);
  signal::getSigaction(SIGWINCH, &sPrevWinch);
  signal::setSignalHandler(SIGTSTP, detail::handleSigtstp);
  signal::setSignalHandler(SIGCONT, detail::handleSigcont);
  signal::setSignalHandler(SIGWINCH, detail::handleSigwinch);
}

void StatusLine::uninstallSignalHandlers() {
  signal::setSigaction(SIGTSTP, &sPrevTstp);
  signal::setSigaction(SIGCONT, &sPrevCont);
  signal::setSigaction(SIGWINCH, &sPrevWinch);
  sActiveInstance = nullptr;
}

void StatusLine::suspend() {
  if (!mEnabled) {
    return;
  }
  deactivate();
}

void StatusLine::resume() {
  if (!isatty(STDOUT_FILENO)) {
    return;
  }
  activate();
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
  writeTty(fmt::format("\n\033[1A\0337\033[1;{}r\0338", mTermRows - 1));
}

void StatusLine::clearScrollRegion() {
  // Reset scroll region to the full terminal (DECSTBM with no args)
  writeTty("\033[r");
}

void StatusLine::handleResize() {
  writeTty(fmt::format("\0337\033[{};1H\033[2K\0338", mTermRows));
  refreshTermSize();
  setScrollRegion();
}

void StatusLine::redraw() {
  if (!mEnabled) {
    return;
  }
  if (mResized) {
    mResized = 0;
    handleResize();
  }
  std::string rendered = renderToString(buildElement());
  // Save cursor, jump to status row, clear line, print rendered content, restore cursor
  writeTty(fmt::format("\0337\033[{};1H\033[2K{}\0338", mTermRows, rendered));
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
  // Uncovered가 발생했을 때만 Survived 옆에 (uncov) 토큰을 표시 — 0일 때는
  // 공백 패딩 없이 바로 다음 토큰이 붙도록 한다.
  std::string uncovToken = mUncovered > 0
      ? fmt::format("({:>{}})", mUncovered, w)
      : std::string{};
  return ftxui::hbox({
      ftxui::text(fmt::format("{}{:>{}} ", Utf8Char::CrossMark, mKilled, w)),
      ftxui::text(fmt::format("{}{:>{}}{} ", Utf8Char::CheckMark, mSurvived, w, uncovToken)),
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
  ftxui::Elements elements;
  elements.push_back(ftxui::text(fmt::format("[{}/{}] ({}%) ", current, mTotal, pct)));
  if (mTotal > 0) {
    float progress = static_cast<float>(current) / static_cast<float>(mTotal);
    elements.push_back(
        ftxui::gauge(progress) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, kGaugeWidth));
    elements.push_back(ftxui::text(" "));
  }
  elements.push_back(ftxui::separatorLight());
  elements.push_back(ftxui::text(" "));
  elements.push_back(buildSummaryElement());
  return ftxui::hbox(std::move(elements));
}

ftxui::Element StatusLine::buildElement() const {
  ftxui::Elements elements;
  if (mDryRun) {
    elements.push_back(ftxui::text(" [DRY-RUN]"));
  }
  if (mTotal > 0) {
    elements.push_back(ftxui::text(" "));
    elements.push_back(ftxui::spinner(kSpinnerCharset, mCurrent));
  }
  elements.push_back(ftxui::text(fmt::format(" {:<{}}", phaseLabel(), kPhaseLabelWidth)));
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

}  // namespace sentinel
