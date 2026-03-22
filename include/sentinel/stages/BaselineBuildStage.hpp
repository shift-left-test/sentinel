/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_BASELINEBUILDSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_BASELINEBUILDSTAGE_HPP_

#include <memory>
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Runs the baseline build. Skips if already completed in a prior run.
 */
class BaselineBuildStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param workspace  Shared workspace.
   */
  BaselineBuildStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine,
                     std::shared_ptr<Workspace> workspace);

 protected:
  bool execute() override;

 private:
  std::shared_ptr<Workspace> mWorkspace;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_BASELINEBUILDSTAGE_HPP_
