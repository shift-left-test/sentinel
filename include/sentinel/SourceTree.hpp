/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_SOURCETREE_HPP_
#define INCLUDE_SENTINEL_SOURCETREE_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Mutant.hpp"

namespace sentinel {

class Mutant;

/**
 * @brief SourceTree class
 */
class SourceTree {
 public:
  /**
   * @brief Default constructor
   *
   * @param baseDirectory repository directory
   */
  explicit SourceTree(const std::filesystem::path& baseDirectory);

  /**
   * @brief Modify the source with respect to the given mutable information
   *
   * @param info Mutant information
   * @param backupPath backup directory
   */
  virtual void modify(const Mutant& info, const std::filesystem::path& backupPath) = 0;

 protected:
  /**
   * @brief Return the base directory of the repository
   *
   * @return base directory
   */
  std::filesystem::path getBaseDirectory() const;

 private:
  std::filesystem::path mBaseDirectory;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_SOURCETREE_HPP_
