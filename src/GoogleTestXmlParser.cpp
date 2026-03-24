/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <string>
#include <string_view>
#include "sentinel/GoogleTestXmlParser.hpp"
#include "sentinel/Logger.hpp"

namespace sentinel {

bool GoogleTestXmlParser::parse(std::shared_ptr<tinyxml2::XMLDocument> document) {
  std::string message = "This file doesn't follow googletest result format";

  tinyxml2::XMLElement* pRoot = document->FirstChildElement("testsuites");
  if (pRoot == nullptr) {
    Logger::debug(message);
    return false;
  }

  tinyxml2::XMLElement* p = pRoot->FirstChildElement("testsuite");
  if (p == nullptr) {
    Logger::debug(message);
    return false;
  }

  for (; p != nullptr; p = p->NextSiblingElement("testsuite")) {
    tinyxml2::XMLElement* q = p->FirstChildElement("testcase");
    if (q == nullptr) {
      Logger::debug(message);
      return false;
    }

    for (; q != nullptr; q = q->NextSiblingElement("testcase")) {
      const char* pStatus = q->Attribute("status");
      if (pStatus == nullptr) {
        Logger::debug(message);
        return false;
      }

      if (std::string_view(pStatus) == "run") {
        const char* pClassName = q->Attribute("classname");
        const char* pName = q->Attribute("name");
        if (pClassName == nullptr || pName == nullptr) {
          Logger::debug(message);
          return false;
        }

        auto savedName = fmt::format("{0}.{1}", pClassName, pName);

        if (q->FirstChildElement("failure") == nullptr) {
          addPassed(savedName);
        } else {
          addFailed(savedName);
        }
      }
    }
  }
  return true;
}

}  // namespace sentinel
