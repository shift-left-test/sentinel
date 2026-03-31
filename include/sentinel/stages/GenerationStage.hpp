/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_GENERATIONSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_GENERATIONSTAGE_HPP_

#include <memory>
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Generates and stores mutants in the workspace. Skips if mutants are already generated.
 */
class GenerationStage : public Stage {
 public:
  /**
   * @brief Constructor.
   * @param repo      Git repository for scanning source lines.
   * @param generator Mutant generator strategy.
   */
  GenerationStage(std::shared_ptr<GitRepository> repo, std::shared_ptr<MutantGenerator> generator);

 protected:
  bool shouldSkip(const PipelineContext& ctx) const override;
  StatusLine::Phase getPhase() const override;
  bool execute(PipelineContext* ctx) override;

 private:
  std::shared_ptr<GitRepository> mRepo;
  std::shared_ptr<MutantGenerator> mGenerator;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_GENERATIONSTAGE_HPP_
