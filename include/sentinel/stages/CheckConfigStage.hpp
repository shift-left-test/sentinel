/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_CHECKCONFIGSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_CHECKCONFIGSTAGE_HPP_

#include <memory>
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Validates required config options and warns about risky settings.
 *        Skips validation when resuming or already-complete.
 */
class CheckConfigStage : public Stage {
 public:
  /**
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param logger     Shared logger.
   * @param workspace  Shared workspace (used to detect run mode).
   */
  CheckConfigStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine,
                   std::shared_ptr<Logger> logger,
                   std::shared_ptr<Workspace> workspace);

 protected:
  bool execute() override;

 private:
  bool checkWarnings();
  std::shared_ptr<Workspace> mWorkspace;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_CHECKCONFIGSTAGE_HPP_
