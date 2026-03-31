/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGE_HPP_
#define INCLUDE_SENTINEL_STAGE_HPP_

#include <memory>
#include "sentinel/PipelineContext.hpp"

namespace sentinel {

/**
 * @brief Abstract base class for pipeline stages (Chain of Responsibility).
 *
 * Subclasses implement execute() to perform their work.
 * run() calls execute() then, if it returns true and a next stage is set,
 * calls next->run() and returns its exit code.
 */
class Stage {
 public:
  Stage();
  Stage(const Stage&) = delete;
  Stage& operator=(const Stage&) = delete;
  virtual ~Stage();

  /**
   * @brief Link the next stage in the chain.
   * @return next, to allow chaining: a->setNext(b)->setNext(c).
   */
  std::shared_ptr<Stage> setNext(std::shared_ptr<Stage> next);

  /**
   * @brief Execute this stage; if execute() returns true and a next stage exists,
   *        invoke it.
   */
  void run(PipelineContext* ctx);

 protected:
  /**
   * @brief Return true if this stage should be skipped.
   */
  virtual bool shouldSkip(const PipelineContext& ctx) const = 0;

  /**
   * @brief Return the StatusLine phase to display when this stage runs.
   */
  virtual StatusLine::Phase getPhase() const = 0;

  /**
   * @brief Perform this stage's work.
   * @return true to continue the chain, false to stop.
   */
  virtual bool execute(PipelineContext* ctx) = 0;

  /** @brief Check whether verbose mode is enabled. */
  static bool isVerbose(const PipelineContext& ctx) { return ctx.config.verbose; }

 private:
  std::shared_ptr<Stage> mNext;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGE_HPP_
