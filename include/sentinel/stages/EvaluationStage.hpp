/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_

#include <memory>
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Evaluates all mutants, recording kill/survive/timeout/build-error results.
 *        Skips if already complete.
 */
class EvaluationStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param workspace  Shared workspace.
   */
  EvaluationStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine, std::shared_ptr<Workspace> workspace);

 protected:
  bool shouldSkip() const override;
  StatusLine::Phase getPhase() const override;
  bool execute() override;

 private:
  std::shared_ptr<Workspace> mWorkspace;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
