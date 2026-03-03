/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_WORKSPACE_HPP_
#define INCLUDE_SENTINEL_WORKSPACE_HPP_

#include <experimental/filesystem>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"

namespace sentinel {

/**
 * @brief Manages the sentinel workspace directory.
 *
 * Directory layout:
 *   &lt;root&gt;/sentinel.yaml          — run options (written after baseline succeeds)
 *   &lt;root&gt;/original/build.log     — baseline build stdout/stderr
 *   &lt;root&gt;/original/test.log      — baseline test stdout/stderr
 *   &lt;root&gt;/original/results/      — baseline test result XML files
 *   &lt;root&gt;/backup/                — temporary backup of mutated source files
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
  explicit Workspace(const std::experimental::filesystem::path& root);

  /**
   * @brief Return true if a previous run exists (sentinel.yaml is present).
   */
  bool hasPreviousRun() const;

  /**
   * @brief Remove all workspace contents and recreate the base directory structure.
   */
  void initialize();

  /**
   * @brief Write YAML-formatted run options to &lt;root&gt;/sentinel.yaml.
   *
   * @param yamlContent  Resolved run options in YAML format.
   */
  void saveConfig(const std::string& yamlContent);

  /** @brief Return the workspace root path. */
  const std::experimental::filesystem::path& getRoot() const;

  /** @brief Return &lt;root&gt;/original/. */
  std::experimental::filesystem::path getOriginalDir() const;

  /** @brief Return &lt;root&gt;/original/results/. */
  std::experimental::filesystem::path getOriginalResultsDir() const;

  /** @brief Return &lt;root&gt;/backup/. */
  std::experimental::filesystem::path getBackupDir() const;

  /** @brief Return &lt;root&gt;/NNNNN/ for the given 1-based mutant ID. */
  std::experimental::filesystem::path getMutantDir(int id) const;

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

 private:
  std::experimental::filesystem::path mRoot;

  /** @brief Return the zero-padded 5-digit directory name for @p id. */
  static std::string mutantDirName(int id);

  /** @brief Return the path to a named file inside &lt;root&gt;/NNNNN/. */
  std::experimental::filesystem::path mutantFile(int id, const std::string& name) const;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_WORKSPACE_HPP_
