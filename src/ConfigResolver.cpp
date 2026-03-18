/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include "sentinel/ConfigResolver.hpp"
#include <algorithm>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace sentinel {

Config ConfigResolver::resolve(const Config& cli, const Config& yaml, const std::string& yamlPath) {
  Config result;

  // 1. Merge core fields with default values
  mergeField(result.sourceDir, cli.sourceDir, yaml.sourceDir, std::string("."));
  mergeField(result.workDir, cli.workDir, yaml.workDir, std::string("./sentinel_workspace"));
  mergeField(result.outputDir, cli.outputDir, yaml.outputDir, std::string(""));
  mergeField(result.verbose, cli.verbose, yaml.verbose, false);
  mergeField(result.silent, cli.silent, yaml.silent, false);
  mergeField(result.debug, cli.debug, yaml.debug, false);
  mergeField(result.force, cli.force, yaml.force, false);

  // 2. Merge build & test fields
  mergeField(result.buildCmd, cli.buildCmd, yaml.buildCmd, std::string(""));
  mergeField(result.compileDbDir, cli.compileDbDir, yaml.compileDbDir, std::string("."));
  mergeField(result.testCmd, cli.testCmd, yaml.testCmd, std::string(""));
  mergeField(result.testResultDir, cli.testResultDir, yaml.testResultDir, std::string(""));
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
  mergeField(result.coverageFiles, cli.coverageFiles, yaml.coverageFiles, std::vector<std::string>{});
  mergeFieldOptional(result.threshold, cli.threshold, yaml.threshold);
  mergeFieldOptional(result.partition, cli.partition, yaml.partition);

  // Special flags are set from CLI exclusively
  result.init = cli.init;
  result.dryRun = cli.dryRun;
  result.noStatusLine = cli.noStatusLine;

  // 4. Path Resolution
  // If we have a yaml file, resolve relative paths from YAML relative to its directory.
  // Otherwise, resolve everything relative to current working directory.
  fs::path yamlBaseDir = yamlPath.empty() ? fs::current_path() : fs::absolute(fs::path(yamlPath)).parent_path();

  auto resolvePath = [&](std::optional<std::string>& optPath, const std::optional<std::string>& cliVal,
                         bool isDir = true) {
    if (optPath && !optPath->empty()) {
      fs::path p(*optPath);
      if (p.is_relative()) {
        if (cliVal.has_value()) {
          *optPath = fs::absolute(p).lexically_normal().string();
        } else {
          *optPath = (yamlBaseDir / p).lexically_normal().string();
        }
      } else {
        *optPath = p.lexically_normal().string();
      }
      if (isDir && !optPath->empty() && optPath->back() != '/') {
        *optPath += "/";
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
      fs::path p((*result.coverageFiles)[i]);
      if (p.is_relative()) {
        // For lists, we need to know where each element came from.
        // Simplified approach: if CLI provided the list, all are relative to current dir.
        if (cli.coverageFiles.has_value()) {
          (*result.coverageFiles)[i] = fs::absolute(p).lexically_normal().string();
        } else {
          (*result.coverageFiles)[i] = (yamlBaseDir / p).lexically_normal().string();
        }
      }
    }
  }

  return result;
}

}  // namespace sentinel
