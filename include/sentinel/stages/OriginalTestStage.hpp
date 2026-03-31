/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_ORIGINALTESTSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_ORIGINALTESTSTAGE_HPP_

#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Runs the original test. Records elapsed time for auto-timeout.
 *        Saves workspace config.yaml after verifying test results.
 *        Skips if already completed in a prior run.
 */
class OriginalTestStage : public Stage {
 protected:
  bool shouldSkip(const PipelineContext& ctx) const override;
  StatusLine::Phase getPhase() const override;
  bool execute(PipelineContext* ctx) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_ORIGINALTESTSTAGE_HPP_
