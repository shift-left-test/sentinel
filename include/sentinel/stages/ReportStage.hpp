/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_REPORTSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_REPORTSTAGE_HPP_

#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Generates XML and HTML mutation reports. Sets exit code 3 if
 *        mutation score is below the configured threshold.
 */
class ReportStage : public Stage {
 protected:
  bool shouldSkip(const PipelineContext& ctx) const override;
  StatusLine::Phase getPhase() const override;
  bool execute(PipelineContext* ctx) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_REPORTSTAGE_HPP_
