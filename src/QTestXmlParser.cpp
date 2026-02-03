/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <memory>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/QTestXmlParser.hpp"

namespace sentinel {

QTestXmlParser::QTestXmlParser() : XmlParser() {
}

bool QTestXmlParser::parse(std::shared_ptr<tinyxml2::XMLDocument> document) {
  auto logger = Logger::getLogger("Result");
  std::string message = "This file doesn't follow QTest result format";

  tinyxml2::XMLElement *p = document->FirstChildElement("testsuite");
  if (p == nullptr) {
    logger->debug(message);
    return false;
  }

  tinyxml2::XMLElement *q = p->FirstChildElement("testcase");
  if (q == nullptr) {
    logger->debug(message);
    return false;
  }

  for ( ; q != nullptr; q = q->NextSiblingElement("testcase")) {
    const char* pResult = q->Attribute("result");
    if (pResult == nullptr) {
      logger->debug(message);
      return false;
    }

    const char* pClassName = p->Attribute("name");
    const char* pName = q->Attribute("name");
    if (pClassName == nullptr || pName == nullptr) {
      logger->debug(message);
      return false;
    }

    auto savedName = fmt::format("{0}.{1}", pClassName, pName);
    if (std::string(pResult) == std::string("pass")) {
      addPassed(savedName);
    } else if (std::string(pResult) == std::string("fail")) {
      addFailed(savedName);
    } else {
      // do nothing
    }
  }

  return true;
}

}  // namespace sentinel
