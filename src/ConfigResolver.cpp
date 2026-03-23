/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/ConfigResolver.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Config ConfigResolver::resolve(const Config& cli, const Config& yaml, const std::filesystem::path& yamlPath) {
  Config result;

  // 1. Merge core fields with default values
  mergeField(result.sourceDir, cli.sourceDir, yaml.sourceDir, fs::path("."));
  mergeField(result.workDir, cli.workDir, yaml.workDir, fs::path("./.sentinel"));
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
  mergeField(result.testResultExts, cli.testResultExts, yaml.testResultExts, std::vector<std::string>{"xml", "XML"});
  mergeField(result.timeout, cli.timeout, yaml.timeout, std::string("auto"));
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
  result.toAbsolutePaths(yamlPath, cli);

  return result;
}

}  // namespace sentinel
