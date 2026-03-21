/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_CHECKCONFIGSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_CHECKCONFIGSTAGE_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Stage.hpp"

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
   * @param workDir    Workspace root path (used to detect run mode).
   */
  CheckConfigStage(const Config& cfg, StatusLine& statusLine,
                   std::shared_ptr<Logger> logger,
                   std::filesystem::path workDir);

 protected:
  bool execute() override;

 private:
  bool checkWarnings();
  std::filesystem::path mWorkDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_CHECKCONFIGSTAGE_HPP_
