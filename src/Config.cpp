/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include <vector>
#include "sentinel/Config.hpp"

namespace sentinel {

namespace fs = std::filesystem;

void Config::toAbsolutePaths(const fs::path& yamlPath, const Config& cliConfig) {
  const fs::path yamlBaseDir = yamlPath.empty() ? fs::current_path() : fs::absolute(yamlPath).parent_path();

  auto resolveOne = [&](fs::path& p, bool fromCli) {
    if (!p.empty()) {
      if (p.is_relative()) {
        p = fromCli ? fs::absolute(p).lexically_normal() : (yamlBaseDir / p).lexically_normal();
      } else {
        p = p.lexically_normal();
      }
    }
  };

  auto resolvePath = [&](std::optional<fs::path>& optPath, const std::optional<fs::path>& cliVal) {
    if (optPath) resolveOne(*optPath, cliVal.has_value());
  };

  resolvePath(sourceDir, cliConfig.sourceDir);
  resolvePath(workDir, cliConfig.workDir);
  resolvePath(outputDir, cliConfig.outputDir);
  resolvePath(compileDbDir, cliConfig.compileDbDir);
  resolvePath(testResultDir, cliConfig.testResultDir);

  if (coverageFiles) {
    bool fromCli = cliConfig.coverageFiles.has_value();
    for (auto& p : *coverageFiles) resolveOne(p, fromCli);
  }
}

}  // namespace sentinel
