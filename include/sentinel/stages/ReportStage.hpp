/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_REPORTSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_REPORTSTAGE_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Generates XML and HTML mutation reports. Sets exit code 3 if
 *        mutation score is below the configured threshold.
 */
class ReportStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param logger     Shared logger.
   * @param workDir    Workspace root path.
   */
  ReportStage(const Config& cfg, StatusLine& statusLine,
              std::shared_ptr<Logger> logger,
              std::filesystem::path workDir);

 protected:
  bool execute() override;

 private:
  std::filesystem::path mWorkDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_REPORTSTAGE_HPP_
