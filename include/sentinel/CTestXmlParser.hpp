/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CTESTXMLPARSER_HPP_
#define INCLUDE_SENTINEL_CTESTXMLPARSER_HPP_

#include <tinyxml2/tinyxml2.h>
#include <memory>
#include "sentinel/XmlParser.hpp"

namespace sentinel {

/**
 * @brief Parse CTest XML files
 */
class CTestXmlParser : public XmlParser {
 public:
  /**
   * @brief Default constructor
   */
  CTestXmlParser();

  /**
   * @brief Default destructor
   */
  virtual ~CTestXmlParser() = default;

 protected:
  bool parse(std::shared_ptr<tinyxml2::XMLDocument> document) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CTESTXMLPARSER_HPP_
