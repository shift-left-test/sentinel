/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONFIG_HPP_
#define INCLUDE_SENTINEL_CONFIG_HPP_

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace sentinel {

/**
 * @brief Parsed partition specification (e.g., "2/4" means part 2 of 4).
 */
struct Partition {
  std::size_t index;  ///< 1-based partition index
  std::size_t count;  ///< Total number of partitions

  /**
   * @brief Parse a "N/TOTAL" string into a Partition.
   *
   * @param s Partition string in the format "N/TOTAL".
   * @return Parsed Partition.
   * @throw std::invalid_argument if the format is invalid.
   */
  static Partition parse(const std::string& s) {
    auto slash = s.find('/');
    if (slash == std::string::npos || slash == 0 || slash + 1 == s.size()) {
      throw std::invalid_argument(fmt::format("Invalid partition value: '{}'. Expected format: N/TOTAL.", s));
    }
    std::size_t idx = 0;
    std::size_t cnt = 0;
    try {
      idx = std::stoul(s.substr(0, slash));
      cnt = std::stoul(s.substr(slash + 1));
    } catch (...) {
      throw std::invalid_argument(
          fmt::format("Invalid partition value: '{}'. N and TOTAL must be positive integers.", s));
    }
    if (cnt == 0 || idx == 0 || idx > cnt) {
      throw std::invalid_argument(fmt::format("Invalid partition value: '{}'. N must be between 1 and TOTAL.", s));
    }
    return {idx, cnt};
  }
};

/**
 * @brief Unified configuration for sentinel.
 *
 * Each field is optional to track whether it was set via CLI or YAML.
 * Missing values are filled with defaults by the ConfigResolver.
 */
struct Config {
  // Common options
  /** @brief Path to the source directory. */
  std::optional<std::filesystem::path> sourceDir;
  /** @brief Path to the working directory. */
  std::optional<std::filesystem::path> workDir;
  /** @brief Path to the output directory for reports. */
  std::optional<std::filesystem::path> outputDir;
  /** @brief Enable verbose output. */
  std::optional<bool> verbose;
  /** @brief Enable silent mode. */
  std::optional<bool> silent;
  /** @brief Enable debug output. */
  std::optional<bool> debug;
  /** @brief Force overwrite of existing files. */
  std::optional<bool> force;

  // Build & Test options
  /** @brief Command used to build the project. */
  std::optional<std::string> buildCmd;
  /** @brief Path to the compilation database directory. */
  std::optional<std::filesystem::path> compileDbDir;
  /** @brief Command used to run tests. */
  std::optional<std::string> testCmd;
  /** @brief Directory where test results are stored. */
  std::optional<std::filesystem::path> testResultDir;
  /** @brief File extensions for test result files. */
  std::optional<std::vector<std::string>> testResultExts;
  /** @brief Time limit for test execution. */
  std::optional<std::string> timeout;  // "auto", "0", or seconds
  /** @brief Time to wait before killing a hung process. */
  std::optional<std::string> killAfter;

  // Mutation options
  /** @brief Scope of mutation (e.g., "commit" or "all"). */
  std::optional<std::string> scope;  // "commit" or "all"
  /** @brief File extensions to consider for mutation. */
  std::optional<std::vector<std::string>> extensions;
  /** @brief Glob patterns for files to include. */
  std::optional<std::vector<std::string>> patterns;
  /** @brief Glob patterns for files to exclude. */
  std::optional<std::vector<std::string>> excludes;
  /** @brief Maximum number of mutants to generate. */
  std::optional<size_t> limit;
  /** @brief Mutant generation strategy. */
  std::optional<std::string> generator;  // "uniform", "random", "weighted"
  /** @brief Random seed for mutant generation. */
  std::optional<unsigned int> seed;
  /** @brief List of mutation operators to apply. */
  std::optional<std::vector<std::string>> operators;
  /** @brief Paths to code coverage information files. */
  std::optional<std::vector<std::filesystem::path>> coverageFiles;
  /** @brief Mutation score threshold for success. */
  std::optional<double> threshold;
  /** @brief Partition for parallel execution (e.g., "N/TOTAL"). */
  std::optional<std::string> partition;  // "N/TOTAL"

  // Special control flags (not usually in YAML)
  /** @brief Initialize sentinel in the current directory. */
  bool init = false;
  /** @brief Run without performing actual mutations. */
  bool dryRun = false;
  /** @brief Disable the status line in the console. */
  bool noStatusLine = false;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CONFIG_HPP_
