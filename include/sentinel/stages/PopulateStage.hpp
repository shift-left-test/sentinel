/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_POPULATESTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_POPULATESTAGE_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Stage.hpp"

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
   * @param logger     Shared logger.
   * @param workDir    Workspace root path.
   */
  PopulateStage(const Config& cfg, StatusLine& statusLine,
                std::shared_ptr<Logger> logger,
                std::filesystem::path workDir);

 protected:
  bool execute() override;

 private:
  std::filesystem::path mWorkDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_POPULATESTAGE_HPP_
