/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONFIG_HPP_
#define INCLUDE_SENTINEL_CONFIG_HPP_

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sentinel {

/**
 * @brief Mutant generation strategy.
 */
enum class Generator { UNIFORM, RANDOM, WEIGHTED };

/**
 * @brief Convert a string to Generator (case-insensitive).
 * @throw std::invalid_argument if the string is not a valid generator.
 */
Generator parseGenerator(const std::string& s);

/**
 * @brief Convert Generator to its string representation.
 */
std::string generatorToString(Generator gen);

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
 * Fields start with sensible defaults. Parsers overwrite fields they find.
 * Path fields are stored as absolute paths after parsing.
 */
struct Config {
  // Common options
  /** @brief Absolute path to the source directory. */
  std::filesystem::path sourceDir;
  /** @brief Absolute path to the working directory. */
  std::filesystem::path workDir;
  /** @brief Absolute path to the output directory for reports (empty = disabled). */
  std::filesystem::path outputDir;

  // Build & Test options
  /** @brief Command used to build the project (empty = not set). */
  std::string buildCmd;
  /** @brief Absolute path to the compilation database directory. */
  std::filesystem::path compileDbDir;
  /** @brief Command used to run tests (empty = not set). */
  std::string testCmd;
  /** @brief Absolute path to directory where test results are stored (empty = not set). */
  std::filesystem::path testResultDir;
  /** @brief Time limit for test execution; nullopt = auto (1.5x baseline + 5s), 0 = no limit. */
  std::optional<std::size_t> timeout;

  // Mutation options
  /** @brief Diff base revision for --from; nullopt = not specified. */
  std::optional<std::string> from;
  /** @brief Include uncommitted changes (staged + unstaged + untracked). */
  bool uncommitted = false;
  /** @brief File extensions to consider for mutation. */
  std::vector<std::string> extensions = {"cxx", "cpp", "cc", "c", "c++", "cu"};
  /** @brief Glob patterns for files to include. */
  std::vector<std::string> patterns;
  /** @brief Mutant generation strategy: UNIFORM, RANDOM, or WEIGHTED. */
  Generator generator = Generator::UNIFORM;
  /** @brief Maximum number of mutants per source line; 0 = unlimited. */
  std::size_t mutantsPerLine = 1;
  /** @brief List of mutation operators to apply (empty = all). */
  std::vector<std::string> operators;
  /** @brief Absolute paths to lcov tracefiles. */
  std::vector<std::filesystem::path> lcovTracefiles;
  /**
   * @brief When true, skip generating mutants for lines not covered by lcovTracefiles.
   *
   * Without this flag, uncovered lines still produce mutants but their evaluation
   * is skipped (kept in the report as SURVIVED*). Requires lcovTracefiles to be set.
   */
  bool restrictGeneration = false;

  // CLI-only run parameters (not read from sentinel.yaml)
  /** @brief Maximum number of mutants to generate; 0 = unlimited (CLI-only). */
  std::size_t limit = 0;
  /** @brief Random seed for mutant generation (CLI-only). */
  std::optional<unsigned int> seed;
  /** @brief Mutation score threshold for success (CLI-only). */
  std::optional<double> threshold;
  /** @brief Partition for parallel execution, e.g., "N/TOTAL" (CLI-only). */
  std::optional<std::string> partition;
  /** @brief Paths to partitioned workspaces to merge (CLI-only). */
  std::vector<std::filesystem::path> mergeWorkspaces;

  // Special control flags
  /** @brief Initialize sentinel in the current directory. */
  bool init = false;
  /** @brief Run without performing actual mutations. */
  bool dryRun = false;
  /** @brief Enable verbose output. */
  bool verbose = false;
  /** @brief Force overwrite of existing files. */
  bool force = false;
  /** @brief Clear workspace and start a fresh run. */
  bool clean = false;

  /**
   * @brief Create a Config with CWD-based default paths filled in.
   * @return Config with sourceDir, workDir, compileDbDir set to absolute CWD-based paths.
   */
  static Config withDefaults();
};

/**
 * @brief Serialize Config to an output stream in YAML format.
 */
std::ostream& operator<<(std::ostream& out, const Config& cfg);

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CONFIG_HPP_
