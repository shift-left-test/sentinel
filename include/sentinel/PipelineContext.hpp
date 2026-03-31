/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_PIPELINECONTEXT_HPP_
#define INCLUDE_SENTINEL_PIPELINECONTEXT_HPP_

#include "sentinel/Config.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Shared pipeline state passed to every stage via execute().
 */
struct PipelineContext {
  const Config& config;      ///< Fully resolved configuration (read-only).
  StatusLine& statusLine;    ///< Shared status line for phase and progress updates.
  Workspace& workspace;      ///< Shared workspace for persistence.
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_PIPELINECONTEXT_HPP_
