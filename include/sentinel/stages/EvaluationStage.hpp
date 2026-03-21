/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/Stage.hpp"
#include "sentinel/StatusLine.hpp"

namespace sentinel {

/**
 * @brief Evaluates all mutants, recording kill/survive/timeout/build-error results.
 *        Skips if already complete. Signal handler globals live in this TU.
 */
class EvaluationStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param logger     Shared logger.
   * @param workDir    Workspace root path.
   */
  EvaluationStage(const Config& cfg, StatusLine& statusLine,
                  std::shared_ptr<Logger> logger,
                  std::filesystem::path workDir);

  /**
   * @brief Restore original source files from backup directory.
   *        Public for signal handler and testability.
   */
  static void restoreBackup(const std::filesystem::path& backup,
                            const std::filesystem::path& srcRoot);

 protected:
  bool execute() override;

 private:
  std::filesystem::path mWorkDir;
};

/**
 * @brief Install signal handlers for the full pipeline.
 *        Must be called once before the pipeline starts (in main).
 *        EvaluationStage will later set backup/source-root globals when known.
 */
void installSignalHandlers(StatusLine* sl, const std::filesystem::path& workDir);

/**
 * @brief Clear the StatusLine pointer in the signal handler.
 *        Called by ReportStage after reports are generated.
 */
void clearSignalStatusLine();

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
