/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_GOOGLETESTXMLPARSER_HPP_
#define INCLUDE_SENTINEL_GOOGLETESTXMLPARSER_HPP_

#include <tinyxml2/tinyxml2.h>
#include <memory>
#include "sentinel/XmlParser.hpp"

namespace sentinel {

/**
 * @brief Parse GoogleTest XML files
 */
class GoogleTestXmlParser : public XmlParser {
 public:
  /**
   * @brief Default constructor
   */
  GoogleTestXmlParser();

  /**
   * @brief Default destructor
   */
  virtual ~GoogleTestXmlParser() = default;

 protected:
  bool parse(std::shared_ptr<tinyxml2::XMLDocument> document) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GOOGLETESTXMLPARSER_HPP_
