/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_WORKSPACE_HPP_
#define INCLUDE_SENTINEL_WORKSPACE_HPP_

#include <filesystem>  // NOLINT
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"

namespace sentinel {

/**
 * @brief Runtime state persisted between pipeline stages in workspace/status.yaml.
 */
struct WorkspaceStatus {
  std::optional<std::size_t> baselineTime;  ///< Computed timeout seconds (timeout:auto only)
  std::optional<std::size_t> candidateCount;  ///< Total candidates before partition
  std::optional<std::size_t> partIndex;  ///< Partition index N (0 = no partition)
  std::optional<std::size_t> partCount;  ///< Partition total (0 = no partition)
};

/**
 * @brief Manages the sentinel workspace directory.
 *
 * Directory layout:
 *   &lt;root&gt;/config.yaml            — run options (written after baseline succeeds)
 *   &lt;root&gt;/original/build.log     — baseline build stdout/stderr
 *   &lt;root&gt;/original/test.log      — baseline test stdout/stderr
 *   &lt;root&gt;/original/results/      — baseline test result XML files
 *   &lt;root&gt;/backup/                — temporary backup of mutated source files
 *   &lt;root&gt;/actual/                — temporary mutant test result XML files (during evaluation)
 *   &lt;root&gt;/run.done               — present when all evaluation is fully complete
 *   &lt;root&gt;/00001/mt.cfg           — mutant data (Mutant::str() format)
 *   &lt;root&gt;/00001/mt.lock          — present while mutant is being processed
 *   &lt;root&gt;/00001/mt.done          — present (with serialized MutationResult) when complete
 *   &lt;root&gt;/00001/build.log        — mutant build stdout/stderr
 *   &lt;root&gt;/00001/test.log         — mutant test stdout/stderr
 */
class Workspace {
 public:
  /**
   * @brief Construct a Workspace rooted at the given directory (need not exist yet).
   */
  explicit Workspace(const std::filesystem::path& root);

  /**
   * @brief Return true if a previous run exists (config.yaml is present).
   */
  bool hasPreviousRun() const;

  /**
   * @brief Remove all workspace contents and recreate the base directory structure.
   */
  void initialize();

  /**
   * @brief Write YAML-formatted run options to &lt;root&gt;/config.yaml.
   *
   * @param yamlContent  Resolved run options in YAML format.
   */
  void saveConfig(const std::string& yamlContent);

  /**
   * @brief Write runtime status fields to workspace/status.yaml (read-modify-write).
   *        Only fields present in @p status are updated; others are preserved.
   * @param status Fields to write.
   */
  void saveStatus(const WorkspaceStatus& status);

  /**
   * @brief Read workspace/status.yaml. Returns empty struct if file does not exist.
   */
  WorkspaceStatus loadStatus() const;

  /** @brief Return the workspace root path. */
  const std::filesystem::path& getRoot() const;

  /** @brief Return &lt;root&gt;/original/. */
  std::filesystem::path getOriginalDir() const;

  /** @brief Return &lt;root&gt;/original/results/. */
  std::filesystem::path getOriginalResultsDir() const;

  /** @brief Return &lt;root&gt;/backup/. */
  std::filesystem::path getBackupDir() const;

  /** @brief Return &lt;root&gt;/actual/ (temporary mutant test results during evaluation). */
  std::filesystem::path getActualDir() const;

  /** @brief Return &lt;root&gt;/original/build.log. */
  std::filesystem::path getOriginalBuildLog() const;

  /** @brief Return &lt;root&gt;/original/test.log. */
  std::filesystem::path getOriginalTestLog() const;

  /**
   * @brief Maximum number of mutants supported.
   *
   * Determined by the 5-digit zero-padded directory name format in mutantDirName().
   * IDs above this value produce a 6-digit name, which breaks the directory naming convention.
   * If this value is changed, the format string in mutantDirName() must be updated too.
   */
  static constexpr int kMaxMutantCount = 99999;

  /** @brief Return &lt;root&gt;/NNNNN/ for the given 1-based mutant ID. */
  std::filesystem::path getMutantDir(int id) const;

  /** @brief Return &lt;root&gt;/NNNNN/build.log for the given 1-based mutant ID. */
  std::filesystem::path getMutantBuildLog(int id) const;

  /** @brief Return &lt;root&gt;/NNNNN/test.log for the given 1-based mutant ID. */
  std::filesystem::path getMutantTestLog(int id) const;

  /**
   * @brief Create &lt;root&gt;/NNNNN/ and write mt.cfg with the mutant's data.
   *
   * @param id  1-based mutant index.
   * @param m   Mutant to persist.
   */
  void createMutant(int id, const Mutant& m);

  /** @brief Return true if &lt;root&gt;/NNNNN/mt.lock exists. */
  bool isLocked(int id) const;

  /** @brief Create &lt;root&gt;/NNNNN/mt.lock (mark as in-progress). */
  void setLock(int id);

  /** @brief Remove &lt;root&gt;/NNNNN/mt.lock. */
  void clearLock(int id);

  /** @brief Return true if &lt;root&gt;/NNNNN/mt.done exists. */
  bool isDone(int id) const;

  /**
   * @brief Return true if the run has fully completed (run.done marker exists).
   */
  bool isComplete() const;

  /**
   * @brief Create the run.done marker to record that evaluation is fully complete.
   */
  void setComplete();

  /**
   * @brief Serialize @p result to &lt;root&gt;/NNNNN/mt.done.
   *
   * @param id      1-based mutant index.
   * @param result  Completed mutation result.
   */
  void setDone(int id, const MutationResult& result);

  /**
   * @brief Deserialize and return the MutationResult from &lt;root&gt;/NNNNN/mt.done.
   *
   * @param id  1-based mutant index.
   * @return Previously stored MutationResult.
   * @throws std::runtime_error if the file cannot be read or parsed.
   */
  MutationResult getDoneResult(int id) const;

  /**
   * @brief Load all mutants from existing NNNNN/mt.cfg files, sorted by ID.
   *
   * @return Vector of (id, Mutant) pairs sorted ascending by id.
   */
  std::vector<std::pair<int, Mutant>> loadMutants() const;

  /**
   * @brief Copy test result files matching @p exts from @p from to @p to.
   *        Clears @p to before copying.
   *
   * @param from Source directory containing test result files.
   * @param to   Destination directory (cleared and recreated).
   * @param exts File extensions to include (empty = all files).
   */
  static void copyTestReportTo(const std::filesystem::path& from, const std::filesystem::path& to,
                               const std::vector<std::string>& exts);

  /**
   * @brief Restore original source files from the backup directory into @p srcRoot.
   *        No-op if the backup directory does not exist or is empty.
   *
   * @param srcRoot Source root to restore files into.
   */
  void restoreBackup(const std::filesystem::path& srcRoot);

 private:
  std::filesystem::path mRoot;

  /** @brief Return the zero-padded 5-digit directory name for @p id. */
  static std::string mutantDirName(int id);

  /** @brief Return the path to a named file inside &lt;root&gt;/NNNNN/. */
  std::filesystem::path mutantFile(int id, const std::string& name) const;

  /** @brief Return &lt;root&gt;/run.done marker path. */
  std::filesystem::path getCompleteMarker() const;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_WORKSPACE_HPP_
