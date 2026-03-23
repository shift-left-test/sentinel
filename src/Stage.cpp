/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <utility>
#include "sentinel/Stage.hpp"

namespace sentinel {

Stage::Stage(const Config& cfg, std::shared_ptr<StatusLine> statusLine) :
    mConfig(cfg), mStatusLine(std::move(statusLine)) {
}

std::shared_ptr<Stage> Stage::setNext(std::shared_ptr<Stage> next) {
  mNext = std::move(next);
  return mNext;
}

void Stage::run() {
  if (!execute()) return;
  if (mNext) mNext->run();
}

}  // namespace sentinel
