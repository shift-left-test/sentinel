/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Stage.hpp"

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
   * @param logger     Shared logger.
   * @param workDir    Workspace root path.
   */
  DryRunStage(const Config& cfg, StatusLine& statusLine,
              std::shared_ptr<Logger> logger,
              std::filesystem::path workDir);

 protected:
  bool execute() override;

 private:
  std::filesystem::path mWorkDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_
