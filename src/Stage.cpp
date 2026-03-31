/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <utility>
#include "sentinel/Stage.hpp"

namespace sentinel {

Stage::Stage() = default;

Stage::~Stage() = default;

std::shared_ptr<Stage> Stage::setNext(std::shared_ptr<Stage> next) {
  mNext = std::move(next);
  return mNext;
}

void Stage::run(PipelineContext* ctx) {
  if (!shouldSkip(*ctx)) {
    ctx->statusLine.setPhase(getPhase());
    try {
      if (!execute(ctx)) {
        ctx->statusLine.disable(); return;
      }
    } catch (...) {
      ctx->statusLine.disable();
      throw;
    }
  }
  if (mNext) mNext->run(ctx);
  else ctx->statusLine.disable();
}

}  // namespace sentinel
