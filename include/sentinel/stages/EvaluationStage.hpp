/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_

#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Result of a single mutant evaluation, including timing.
 */
struct EvaluationDetail {
  MutationResult result;  ///< Mutation evaluation result
  double buildSecs;       ///< Build duration in seconds
  double testSecs;        ///< Test duration in seconds
};

/**
 * @brief Evaluates all mutants, recording kill/survive/timeout/build-error results.
 *        Skips if already complete.
 */
class EvaluationStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param cfg        Resolved configuration.
   * @param statusLine Shared status line.
   * @param workspace  Shared workspace.
   */
  EvaluationStage(const Config& cfg, std::shared_ptr<StatusLine> statusLine, std::shared_ptr<Workspace> workspace);

 protected:
  bool shouldSkip() const override;
  StatusLine::Phase getPhase() const override;
  bool execute() override;

 private:
  std::shared_ptr<Workspace> mWorkspace;

  /**
   * @brief Apply a mutant, run build/test, compare results, then restore backup.
   *
   * @param m              Mutant to evaluate.
   * @param id             1-based mutant ID (used for log paths).
   * @param timeLimit      Test timeout in seconds (0 = no limit).
   * @param killAfterSecs  Seconds to wait before force-killing a timed-out process.
   * @param evaluator      Evaluator used to compare expected vs actual results.
   * @return EvaluationDetail containing the result and build/test durations.
   */
  EvaluationDetail evaluateMutant(const Mutant& m, int id, std::size_t timeLimit, std::size_t killAfterSecs,
                                  Evaluator* evaluator);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
