/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <utility>
#include "sentinel/Stage.hpp"

namespace sentinel {

Stage::~Stage() = default;

Stage::Stage(const Config& cfg, std::shared_ptr<StatusLine> statusLine) :
    mConfig(cfg), mStatusLine(std::move(statusLine)) {
}

std::shared_ptr<Stage> Stage::setNext(std::shared_ptr<Stage> next) {
  mNext = std::move(next);
  return mNext;
}

void Stage::run() {
  if (!shouldSkip()) {
    mStatusLine->setPhase(getPhase());
    try {
      if (!execute()) {
        mStatusLine->disable(); return;
      }
    } catch (...) {
      mStatusLine->disable();
      throw;
    }
  }
  if (mNext) mNext->run();
  else mStatusLine->disable();
}

}  // namespace sentinel
