/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_ORIGINALBUILDSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_ORIGINALBUILDSTAGE_HPP_

#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Runs the original build. Skips if already completed in a prior run.
 */
class OriginalBuildStage : public Stage {
 protected:
  bool shouldSkip(const PipelineContext& ctx) const override;
  StatusLine::Phase getPhase() const override;
  bool execute(PipelineContext* ctx) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_ORIGINALBUILDSTAGE_HPP_
