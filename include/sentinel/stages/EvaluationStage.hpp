/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_

#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Stage.hpp"

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
   * @param repo Git repository for applying/reverting source patches.
   */
  explicit EvaluationStage(std::shared_ptr<GitRepository> repo);

 protected:
  bool shouldSkip(const PipelineContext& ctx) const override;
  StatusLine::Phase getPhase() const override;
  bool execute(PipelineContext* ctx) override;

 private:
  std::shared_ptr<GitRepository> mRepo;

  /**
   * @brief Apply a mutant, run build/test, compare results, then restore backup.
   */
  EvaluationDetail evaluateMutant(const Mutant& m, int id, std::size_t timeLimit, std::size_t killAfterSecs,
                                  Evaluator* evaluator, PipelineContext* ctx);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_EVALUATIONSTAGE_HPP_
