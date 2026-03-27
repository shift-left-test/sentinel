/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include "sentinel/Config.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Config Config::withDefaults() {
  Config cfg;
  cfg.sourceDir = fs::absolute(".").lexically_normal();
  cfg.workDir = fs::absolute(".sentinel").lexically_normal();
  cfg.compileDbDir = fs::absolute(".").lexically_normal();
  return cfg;
}

}  // namespace sentinel
