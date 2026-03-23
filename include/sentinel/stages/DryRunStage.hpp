/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_

#include <memory>
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Prints a dry-run summary and stops the chain if --dry-run is set.
 *        Otherwise passes through transparently.
 */
class DryRunStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param workspace  Shared workspace.
   */
  DryRunStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine, std::shared_ptr<Workspace> workspace);

 protected:
  bool shouldSkip() const override;
  StatusLine::Phase getPhase() const override;
  bool execute() override;

 private:
  std::shared_ptr<Workspace> mWorkspace;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_
