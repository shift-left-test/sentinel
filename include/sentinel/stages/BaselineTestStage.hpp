/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_BASELINETESTSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_BASELINETESTSTAGE_HPP_

#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Runs the baseline test. Records elapsed time for auto-timeout.
 *        Saves workspace config.yaml after verifying test results.
 *        Skips if already completed in a prior run.
 */
class BaselineTestStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param logger     Shared logger.
   * @param workspace  Shared workspace.
   */
  BaselineTestStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine,
                    std::shared_ptr<Logger> logger,
                    std::shared_ptr<Workspace> workspace);

  /**
   * @brief Copy test result files matching @p exts from @p from to @p to.
   *        Clears @p to before copying. Public for testability.
   */
  static void copyTestReportTo(const std::filesystem::path& from,
                                const std::filesystem::path& to,
                                const std::vector<std::string>& exts);

 protected:
  bool execute() override;

 private:
  std::shared_ptr<Workspace> mWorkspace;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_BASELINETESTSTAGE_HPP_
