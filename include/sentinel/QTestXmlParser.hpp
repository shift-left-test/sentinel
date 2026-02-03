/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_QTESTXMLPARSER_HPP_
#define INCLUDE_SENTINEL_QTESTXMLPARSER_HPP_

#include <tinyxml2/tinyxml2.h>
#include <memory>
#include "sentinel/XmlParser.hpp"

namespace sentinel {

/**
 * @brief Parse QTest XML files
 */
class QTestXmlParser : public XmlParser {
 public:
  /**
   * @brief Default constructor
   */
  QTestXmlParser();

  /**
   * @brief Default destructor
   */
  virtual ~QTestXmlParser() = default;

 protected:
  bool parse(std::shared_ptr<tinyxml2::XMLDocument> document) override;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_QTESTXMLPARSER_HPP_
