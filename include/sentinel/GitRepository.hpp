/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_GITREPOSITORY_HPP_
#define INCLUDE_SENTINEL_GITREPOSITORY_HPP_

#include <memory>
#include <string>
#include <vector>
#include "sentinel/exceptions/RepositoryException.hpp"
#include "sentinel/Repository.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/SourceTree.hpp"
#include "sentinel/Logger.hpp"

namespace sentinel {

/**
 * @brief GitRepository class
 */
class GitRepository : public Repository {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to the directory repository
   * @param extensions list of allowed extensions for target files
   * @param patterns list of patterns to constrain diff
   * @param excludes excluded directory list
   */
  explicit GitRepository(const std::string& path,
                         const std::vector<std::string>& extensions = std::vector<std::string>(),
                         const std::vector<std::string>& patterns = std::vector<std::string>(),
                         const std::vector<std::string>& excludes = std::vector<std::string>());
  GitRepository(const GitRepository&) = delete;
  GitRepository& operator=(const GitRepository&) = delete;

  /**
   * @brief Default destructor
   */
  virtual ~GitRepository();

  std::shared_ptr<SourceTree> getSourceTree() override;
  /**
   * @brief Return the diff source lines from commit tree.
   *
   * @return SourceLines object
   */
  SourceLines getSourceLines(const std::string& scope) override;

  /**
   * @brief Return absolute root path
   */
  const std::filesystem::path& getSourceRoot() {
    return mSourceRoot;
  }

  /**
   * @brief Return path is target path for getSourceLines
   *
   * @param path
   *
   * @param checkExtension if true,
   *        checks whether the file extension of the path is
   *        included in extensions.
   *
   * @return return true if path is valid sourceline target.
   */
  bool isTargetPath(const std::filesystem::path& path, bool checkExtension = true);

  /**
   * @brief Add a directory to skip during git repository discovery.
   *
   * Any directory matching @p dir (by canonical path) will not be traversed
   * when searching for nested git repositories in getSourceLines().  Use this
   * to prevent sentinel-managed directories (workspace, output, etc.) from
   * being scanned unnecessarily.
   *
   * @param dir absolute or relative path of the directory to skip
   */
  void addSkipDir(const std::filesystem::path& dir);

 private:
  std::filesystem::path mSourceRoot;
  std::vector<std::string> mExtensions;
  std::vector<std::string> mPatterns;
  std::vector<std::string> mExcludes;
  std::vector<std::filesystem::path> mSkipDirs;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GITREPOSITORY_HPP_
