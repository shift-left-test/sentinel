/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_PARTITIONEDWORKSPACEMERGER_HPP_
#define INCLUDE_SENTINEL_PARTITIONEDWORKSPACEMERGER_HPP_

#include <filesystem>  // NOLINT
#include <map>
#include <string>
#include <vector>
#include "sentinel/Workspace.hpp"

namespace sentinel {

/**
 * @brief Merges partitioned workspace results into a single target workspace.
 *
 * Validates source workspaces, checks config compatibility, copies mutant
 * result files, and tracks merge completeness.
 */
class PartitionedWorkspaceMerger {
 public:
  /**
   * @brief Construct a merger.
   *
   * @param targetDir  Path to the target (merged) workspace.
   * @param sourceDirs Paths to partitioned source workspaces.
   * @param force      Allow overwriting identical existing files.
   */
  PartitionedWorkspaceMerger(const std::filesystem::path& targetDir,
                             const std::vector<std::filesystem::path>& sourceDirs,
                             bool force);

  /**
   * @brief Execute the merge operation.
   *
   * Validates sources, checks compatibility, copies mutant results,
   * updates completeness tracking, and prints a summary.
   *
   * @throws std::runtime_error on validation or merge errors.
   */
  void merge();

 private:
  std::filesystem::path mTargetDir;
  std::vector<std::filesystem::path> mSourceDirs;
  bool mForce;
  mutable std::map<std::filesystem::path, WorkspaceStatus> mStatusCache;

  void validateSource(const std::filesystem::path& sourceDir) const;
  void validateCompatibility() const;
  void prepareTargetWorkspace() const;
  void copyBaseline(const std::filesystem::path& sourceDir) const;
  void copyMutants(const std::filesystem::path& sourceDir) const;
  void copyFileWithConflictCheck(const std::filesystem::path& src,
                                 const std::filesystem::path& dst) const;
  void updateCompleteness() const;

  static std::string loadConfigWithoutExcludedFields(
      const std::filesystem::path& sourceDir);
  static std::string readFileContent(const std::filesystem::path& path);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_PARTITIONEDWORKSPACEMERGER_HPP_
