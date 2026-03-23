/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONSUMMARY_HPP_
#define INCLUDE_SENTINEL_MUTATIONSUMMARY_HPP_

#include <filesystem>  // NOLINT
#include <map>
#include <vector>
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"

namespace sentinel {

/**
 * @brief Aggregated statistics computed from MutationResults.
 *
 * Serves as the Model for report generation: all Views (XmlReport, HtmlReport,
 * printSummary) consume a MutationSummary rather than raw MutationResults.
 */
struct MutationSummary {
  /**
   * @brief Aggregated statistics for mutations in a directory.
   */
  struct DirStats {
    std::vector<const MutationResult*> results;  ///< All mutation results in the directory
    std::size_t total = 0;   ///< Total number of mutations
    std::size_t detected = 0;  ///< Number of detected (killed) mutations
    std::size_t fileCount = 0;  ///< Number of distinct source files
  };

  /**
   * @brief Aggregated statistics for mutations in a single source file.
   */
  struct FileStats {
    std::vector<const MutationResult*> results;  ///< All mutation results in the file
    std::size_t total = 0;   ///< Total number of mutations
    std::size_t detected = 0;  ///< Number of detected (killed) mutations
  };

  MutationResults results;  ///< Raw mutation results
  std::filesystem::path sourcePath;  ///< Absolute path to the source root

  std::map<std::filesystem::path, DirStats> groupByDirPath;  ///< Results grouped by directory
  std::map<std::filesystem::path, FileStats> groupByPath;  ///< Results grouped by file

  std::size_t totNumberOfMutation = 0;  ///< Total evaluated mutations (excl. skipped)
  std::size_t totNumberOfDetectedMutation = 0;  ///< Total killed mutations
  std::size_t totNumberOfBuildFailure = 0;  ///< Total build failures
  std::size_t totNumberOfRuntimeError = 0;  ///< Total runtime errors
  std::size_t totNumberOfTimeout = 0;  ///< Total timeouts

  /**
   * @brief Construct and aggregate from in-memory results.
   *
   * @param results    Mutation results to aggregate.
   * @param sourcePath Absolute path to the source root directory.
   * @throw InvalidArgumentException if @p sourcePath does not exist.
   */
  MutationSummary(const MutationResults& results, const std::filesystem::path& sourcePath);

  /**
   * @brief Construct and aggregate by loading results from a file.
   *
   * @param resultsPath Path to the serialized MutationResults file.
   * @param sourcePath  Absolute path to the source root directory.
   * @throw InvalidArgumentException if @p sourcePath does not exist.
   */
  MutationSummary(const std::filesystem::path& resultsPath, const std::filesystem::path& sourcePath);

  /**
   * @brief Copy constructor — remaps internal pointers to the new results copy.
   */
  MutationSummary(const MutationSummary& other);

  /** @brief Copy assignment */
  MutationSummary& operator=(MutationSummary other);

  /** @brief Move constructor — internal pointers remain valid after the move. */
  MutationSummary(MutationSummary&&) = default;

  /** @brief Move assignment */
  MutationSummary& operator=(MutationSummary&&) = default;

  /**
   * @brief Return the relative path from @p start to @p path.
   */
  static std::filesystem::path getRelativePath(const std::filesystem::path& path,
                                               const std::filesystem::path& start);

 private:
  void aggregate();
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONSUMMARY_HPP_
