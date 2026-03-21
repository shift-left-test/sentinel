/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_BASELINEBUILDSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_BASELINEBUILDSTAGE_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Stage.hpp"

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
   * @param logger     Shared logger.
   * @param workDir    Workspace root path.
   */
  BaselineBuildStage(const Config& cfg, StatusLine& statusLine,
                     std::shared_ptr<Logger> logger,
                     std::filesystem::path workDir);

 protected:
  bool execute() override;

 private:
  std::filesystem::path mWorkDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_BASELINEBUILDSTAGE_HPP_
