/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_POPULATESTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_POPULATESTAGE_HPP_

#include <memory>
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Generates and stores mutants in the workspace. Skips if already populated.
 */
class PopulateStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param workspace  Shared workspace.
   */
  PopulateStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine, std::shared_ptr<Workspace> workspace);

 protected:
  bool shouldSkip() const override;
  StatusLine::Phase getPhase() const override;
  bool execute() override;

 private:
  std::shared_ptr<Workspace> mWorkspace;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_POPULATESTAGE_HPP_
