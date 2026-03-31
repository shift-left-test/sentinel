/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_

#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Prints a dry-run summary and stops the chain if --dry-run is set.
 *        Otherwise passes through transparently.
 */
class DryRunStage : public Stage {
 protected:
  bool shouldSkip(const PipelineContext& ctx) const override;
  StatusLine::Phase getPhase() const override;
  bool execute(PipelineContext* ctx) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_DRYRUNSTAGE_HPP_
