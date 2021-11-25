/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_DOCGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_DOCGENERATOR_HPP_

#include <string>

namespace sentinel {

/**
 * @brief DOCGenerator interface
 */
class DOCGenerator {
 public:
  /**
   * @brief make DOC content
   *
   * @return content string
   */
  virtual std::string str() = 0;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_DOCGENERATOR_HPP_
