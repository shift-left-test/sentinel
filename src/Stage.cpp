/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <utility>
#include "sentinel/Stage.hpp"

namespace sentinel {

Stage::Stage(const Config& cfg, StatusLine& statusLine, std::shared_ptr<Logger> logger)
    : mConfig(cfg), mStatusLine(statusLine), mLogger(std::move(logger)) {}

std::shared_ptr<Stage> Stage::setNext(std::shared_ptr<Stage> next) {
  mNext = std::move(next);
  return mNext;
}

int Stage::run() {
  if (!execute()) return mExitCode;
  if (mNext) return mNext->run();
  return mExitCode;
}

}  // namespace sentinel
