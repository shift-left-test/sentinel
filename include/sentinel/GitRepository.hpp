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
   * @param extensions compile_commands.json file location
   * @param excludes excluded directory list
   */
  explicit GitRepository(const std::string& path,
    const std::vector<std::string>& extensions = std::vector<std::string>(),
    const std::vector<std::string>& excludes = std::vector<std::string>());

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
  const std::experimental::filesystem::path& getSourceRoot() {
    return mSourceRoot;
  }

  /**
   * @brief Return path is target path for getSourceLines
   *
   * @param path
   *
   * @param checkExtension if true,
   *        check extensino of path is included by extensions_.
   *
   * @return return true if path is valid sourceline target.
   */
  bool isTargetPath(const std::experimental::filesystem::path &path,
    bool checkExtension = true);

 private:
  std::experimental::filesystem::path mSourceRoot;
  std::vector<std::string> mExtensions;
  std::vector<std::string> mExcludes;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GITREPOSITORY_HPP_
