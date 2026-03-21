/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_BASELINETESTSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_BASELINETESTSTAGE_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/Stage.hpp"

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
   * @param workDir    Workspace root path.
   */
  BaselineTestStage(const Config& cfg, StatusLine& statusLine,
                    std::shared_ptr<Logger> logger,
                    std::filesystem::path workDir);

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
  std::filesystem::path mWorkDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_BASELINETESTSTAGE_HPP_
