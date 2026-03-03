/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_SENTINELCONFIG_HPP_
#define INCLUDE_SENTINEL_SENTINELCONFIG_HPP_

#include <optional>
#include <string>
#include <vector>

namespace sentinel {

/**
 * @brief Configuration loaded from a YAML config file (sentinel.yaml).
 *
 * Each field is optional; only fields present in the YAML file are populated.
 * CLI arguments always take priority over values from this struct.
 */
struct SentinelConfig {
  // Shared with Command base class
  /// Path to the source root directory (--source-root).
  std::optional<std::string> sourceRoot;
  /// Enable verbose logging (--verbose).
  std::optional<bool> verbose;
  /// Enable debug logging (--debug).
  std::optional<bool> debug;
  /// Workspace directory for sentinel run artifacts (--workspace).
  std::optional<std::string> workDir;
  /// Directory for output reports (--output-dir).
  std::optional<std::string> outputDir;
  /// Change to this directory before running (--cwd).
  std::optional<std::string> cwd;

  // CommandRun-specific options
  /// Build command output directory (--build-dir).
  std::optional<std::string> buildDir;
  /// Path to directory containing compile_commands.json (--compiledb).
  std::optional<std::string> compileDbDir;
  /// Diff scope, one of ['commit', 'all'] (--scope).
  std::optional<std::string> scope;
  /// Source file extensions to mutate (--extension).
  std::optional<std::vector<std::string>> extensions;
  /// Paths or patterns to constrain the diff (--pattern).
  std::optional<std::vector<std::string>> patterns;
  /// Paths excluded from mutation (--exclude).
  std::optional<std::vector<std::string>> excludes;
  /// Maximum number of mutants to generate (--limit).
  std::optional<size_t> limit;
  /// Shell command to build the source (--build-command).
  std::optional<std::string> buildCmd;
  /// Shell command to execute tests (--test-command).
  std::optional<std::string> testCmd;
  /// Directory containing test command output (--test-result-dir).
  std::optional<std::string> testResultDir;
  /// Test result file extensions (--test-result-extension).
  std::optional<std::vector<std::string>> testResultFileExts;
  /// lcov-format coverage result files (--coverage).
  std::optional<std::vector<std::string>> coverageFiles;
  /// Mutant generator type, one of ['uniform', 'random', 'weighted'] (--generator).
  std::optional<std::string> generator;
  /// Time limit in seconds for the test command (--timeout).
  std::optional<std::string> timeLimit;
  /// Seconds after timeout before SIGKILL is sent (--kill-after).
  std::optional<std::string> killAfter;
  /// Random seed for mutant selection (--seed).
  std::optional<unsigned> seed;
  /// Mutation operators to use (--operator).
  std::optional<std::vector<std::string>> operators;

  /**
   * @brief Load configuration from a YAML file.
   *
   * Keys in the YAML file correspond to CLI long option names (e.g.,
   * "build-dir", "test-command"). Only known keys are parsed; unknown keys
   * are silently ignored. Missing keys leave the corresponding optional empty.
   *
   * @param path Path to the YAML configuration file.
   * @return Parsed SentinelConfig with populated optional fields.
   * @throws std::runtime_error on YAML parse errors or type conversion failures.
   */
  static SentinelConfig loadFromFile(const std::string& path);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_SENTINELCONFIG_HPP_
