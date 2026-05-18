/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_GITSOURCETREE_HPP_
#define INCLUDE_SENTINEL_GITSOURCETREE_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include <string_view>
#include "sentinel/Mutant.hpp"
#include "sentinel/SourceTree.hpp"

namespace sentinel {

/**
 * @brief SourceTree class
 */
class GitSourceTree : public SourceTree {
 public:
  /**
   * @brief Suffix appended to the target path while modify() writes the
   *        mutated content out-of-place. Exposed so tests can assert the
   *        atomic-rename invariant without duplicating the literal.
   */
  static constexpr std::string_view kMutatedTempSuffix = ".sentinel_mutated";

  /**
   * @brief Default constructor
   *
   * @param baseDirectory base directory
   */
  explicit GitSourceTree(const std::filesystem::path& baseDirectory);

  void modify(const Mutant& info, const std::filesystem::path& backupPath) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GITSOURCETREE_HPP_
