/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONRUNNER_HPP_
#define INCLUDE_SENTINEL_MUTATIONRUNNER_HPP_

#include <cstddef>
#include <filesystem>  // NOLINT
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Config.hpp"

namespace sentinel {

/**
 * @brief The core engine that runs the mutation testing pipeline.
 */
class MutationRunner {
 public:
  /**
   * @brief Constructor — initializes with resolved configuration.
   * @param config The finalized configuration object.
   */
  explicit MutationRunner(const Config& config);
  MutationRunner(const MutationRunner&) = delete;
  MutationRunner& operator=(const MutationRunner&) = delete;

  /**
   * @brief Destructor
   */
  virtual ~MutationRunner() = default;

  /**
   * @brief Applies configuration-based settings like log levels.
   */
  void init();

  /**
   * @brief Executes the mutation testing pipeline.
   * @return 0 on success, non-zero on error.
   */
  int run();

  /**
   * @brief Restores source files modified by a mutant from the backup directory.
   * @param backup  Path to the backup directory containing original files.
   * @param srcRoot Path to the source root where files are restored.
   */
  static void restoreBackup(const std::string& backup, const std::string& srcRoot);

  /**
   * @brief Copies test result files with the given extensions from @p from to @p to.
   * @param from Source directory containing test result files.
   * @param to   Destination directory to copy files into.
   * @param exts File extensions to copy (e.g. {"xml", "XML"}).
   */
  static void copyTestReportTo(const std::string& from, const std::string& to,
                                const std::vector<std::string>& exts);

 protected:
  /**
   * @brief Installs signal handlers that restore the backup on abnormal exit.
   */
  virtual void setSignalHandler();

  /**
   * @brief Ensures @p target exists as a directory and returns its canonical path.
   *
   * @param target      Directory path to prepare.
   * @param targetExists Set to @c true if the directory already existed, @c false otherwise.
   * @param isFilledDir  When @c false, throws if the directory is non-empty.
   * @return Canonical absolute path of @p target.
   */
  std::string preProcessWorkDir(const std::string& target, bool* targetExists, bool isFilledDir);

 private:
  /**
   * @brief Checks resolved options for potentially problematic settings and
   *        asks the user to confirm before starting the pipeline.
   *
   * @return @c true if the run should proceed, @c false if the user aborted.
   */
  bool checkConfigWarnings();

  /** @brief Finalized configuration object. */
  Config mConfig;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRUNNER_HPP_
