/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_REPOSITORY_HPP_
#define INCLUDE_SENTINEL_REPOSITORY_HPP_

#include <memory>
#include <string>
#include "sentinel/SourceLines.hpp"
#include "sentinel/SourceTree.hpp"


namespace sentinel {

/**
 * @brief Repository interface
 */
class Repository {
 public:
  /**
   * @brief destructor
   */
  virtual ~Repository() = default;

  /**
   * @brief Return the diff source lines from repository.
   *
   * @return SourceLines object
   */
  virtual SourceLines getSourceLines(const std::string& scope) = 0;

  /**
   * @brief Return the source tree
   *
   * @return SourceTree object
   */
  virtual std::shared_ptr<SourceTree> getSourceTree() = 0;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_REPOSITORY_HPP_
