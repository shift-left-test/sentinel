/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_DOCGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_DOCGENERATOR_HPP_

#include <string>

namespace sentinel {

/**
 * @brief DocGenerator interface
 */
class DocGenerator {
 public:
  /**
   * @brief Default destructor
   */
  virtual ~DocGenerator();

  /**
   * @brief make DOC content
   *
   * @return content string
   */
  virtual std::string str() const = 0;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_DOCGENERATOR_HPP_
