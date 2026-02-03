/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <memory>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/CTestXmlParser.hpp"

namespace sentinel {

CTestXmlParser::CTestXmlParser() : XmlParser() {
}

bool CTestXmlParser::parse(std::shared_ptr<tinyxml2::XMLDocument> document) {
  auto logger = Logger::getLogger("Result");
  std::string message = "This file doesn't follow ctest result format";

  tinyxml2::XMLElement *p = document->FirstChildElement("testsuite");
  if (p == nullptr) {
    logger->debug(message);
    return false;
  }

  for ( ; p != nullptr; p = p->NextSiblingElement("testsuite")) {
    tinyxml2::XMLElement *q = p->FirstChildElement("testcase");
    if (q == nullptr) {
      logger->debug(message);
      return false;
    }

    for ( ; q != nullptr; q = q->NextSiblingElement("testcase")) {
      const char* pStatus = q->Attribute("status");
      if (pStatus == nullptr) {
        logger->debug(message);
        return false;
      }

      if (std::string(pStatus) == std::string("run") ||
          std::string(pStatus) == std::string("fail")) {
        const char* pName = q->Attribute("name");
        if (pName == nullptr) {
          logger->debug(message);
          return false;
        }

        if (q->FirstChildElement("failure") == nullptr) {
          addPassed(std::string(pName));
        } else {
          addFailed(std::string(pName));
        }
      }
    }
  }

  return true;
}

}  // namespace sentinel
