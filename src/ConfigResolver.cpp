/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/ConfigResolver.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Config ConfigResolver::resolve(const Config& cli, const Config& yaml,
                               const std::filesystem::path& yamlPath) {
  Config result;

  // 1. Merge core fields with default values
  mergeField(result.sourceDir, cli.sourceDir, yaml.sourceDir, fs::path("."));
  mergeField(result.workDir, cli.workDir, yaml.workDir, fs::path("./sentinel_workspace"));
  mergeField(result.outputDir, cli.outputDir, yaml.outputDir, fs::path(""));
  mergeField(result.verbose, cli.verbose, yaml.verbose, false);
  mergeField(result.silent, cli.silent, yaml.silent, false);
  mergeField(result.debug, cli.debug, yaml.debug, false);
  mergeField(result.force, cli.force, yaml.force, false);

  // 2. Merge build & test fields
  mergeField(result.buildCmd, cli.buildCmd, yaml.buildCmd, std::string(""));
  mergeField(result.compileDbDir, cli.compileDbDir, yaml.compileDbDir, fs::path("."));
  mergeField(result.testCmd, cli.testCmd, yaml.testCmd, std::string(""));
  mergeField(result.testResultDir, cli.testResultDir, yaml.testResultDir, fs::path(""));
  mergeField(result.testResultFileExts, cli.testResultFileExts, yaml.testResultFileExts,
             std::vector<std::string>{"xml", "XML"});
  mergeField(result.timeLimit, cli.timeLimit, yaml.timeLimit, std::string("auto"));
  mergeField(result.killAfter, cli.killAfter, yaml.killAfter, std::string("60"));

  // 3. Merge mutation options
  mergeField(result.scope, cli.scope, yaml.scope, std::string("all"));
  mergeField(result.extensions, cli.extensions, yaml.extensions,
             std::vector<std::string>{"cxx", "cpp", "cc", "c", "c++", "cu"});
  mergeField(result.patterns, cli.patterns, yaml.patterns, std::vector<std::string>{});
  mergeField(result.excludes, cli.excludes, yaml.excludes, std::vector<std::string>{});
  mergeField(result.limit, cli.limit, yaml.limit, static_cast<size_t>(0));
  mergeField(result.generator, cli.generator, yaml.generator, std::string("uniform"));
  mergeFieldOptional(result.seed, cli.seed, yaml.seed);
  mergeField(result.operators, cli.operators, yaml.operators, std::vector<std::string>{});
  mergeField(result.coverageFiles, cli.coverageFiles, yaml.coverageFiles, std::vector<fs::path>{});
  mergeFieldOptional(result.threshold, cli.threshold, yaml.threshold);
  mergeFieldOptional(result.partition, cli.partition, yaml.partition);

  // Special flags are set from CLI exclusively
  result.init = cli.init;
  result.dryRun = cli.dryRun;
  result.noStatusLine = cli.noStatusLine;

  // 4. Path Resolution
  // If we have a yaml file, resolve relative paths from YAML relative to its directory.
  // Otherwise, resolve everything relative to current working directory.
  fs::path yamlBaseDir = yamlPath.empty() ? fs::current_path() : fs::absolute(yamlPath).parent_path();

  auto resolvePath = [&](std::optional<fs::path>& optPath, const std::optional<fs::path>& cliVal) {
    if (optPath && !optPath->empty()) {
      fs::path p = *optPath;
      if (p.is_relative()) {
        if (cliVal.has_value()) {
          *optPath = fs::absolute(p).lexically_normal();
        } else {
          *optPath = (yamlBaseDir / p).lexically_normal();
        }
      } else {
        *optPath = p.lexically_normal();
      }
    }
  };

  resolvePath(result.sourceDir, cli.sourceDir);
  resolvePath(result.workDir, cli.workDir);
  resolvePath(result.outputDir, cli.outputDir);
  resolvePath(result.compileDbDir, cli.compileDbDir);
  resolvePath(result.testResultDir, cli.testResultDir);

  if (result.coverageFiles) {
    for (size_t i = 0; i < result.coverageFiles->size(); ++i) {
      fs::path p = (*result.coverageFiles)[i];
      if (p.is_relative()) {
        if (cli.coverageFiles.has_value()) {
          (*result.coverageFiles)[i] = fs::absolute(p).lexically_normal();
        } else {
          (*result.coverageFiles)[i] = (yamlBaseDir / p).lexically_normal();
        }
      } else {
        (*result.coverageFiles)[i] = p.lexically_normal();
      }
    }
  }

  return result;
}

}  // namespace sentinel
