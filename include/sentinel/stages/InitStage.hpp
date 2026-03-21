/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_STAGES_INITSTAGE_HPP_
#define INCLUDE_SENTINEL_STAGES_INITSTAGE_HPP_

#include "sentinel/Stage.hpp"

namespace sentinel {

/**
 * @brief Handles the --init flag: writes sentinel.yaml template and stops the chain.
 *        If --init is not set, passes through transparently.
 */
class InitStage : public Stage {
 public:
  /**
   * @brief Constructor (inherited from Stage).
   */
  using Stage::Stage;

 protected:
  bool execute() override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_STAGES_INITSTAGE_HPP_
