/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_GITSOURCETREE_HPP_
#define INCLUDE_SENTINEL_GITSOURCETREE_HPP_

#include <experimental/filesystem>
#include <string>
#include "sentinel/Mutant.hpp"
#include "sentinel/SourceTree.hpp"


namespace sentinel {

/**
 * @brief SourceTree class
 */
class GitSourceTree : public SourceTree {
 public:
  /**
   * @brief Default constructor
   *
   * @param baseDirectory base directory
   */
  explicit GitSourceTree(const std::string& baseDirectory);

  void modify(const Mutant& info,
      const std::experimental::filesystem::path& backupPath) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GITSOURCETREE_HPP_
