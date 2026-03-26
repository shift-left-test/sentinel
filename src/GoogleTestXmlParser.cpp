/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <string>
#include <string_view>
#include "sentinel/GoogleTestXmlParser.hpp"

namespace sentinel {

bool GoogleTestXmlParser::parse(std::shared_ptr<tinyxml2::XMLDocument> document) {
  tinyxml2::XMLElement* pRoot = document->FirstChildElement("testsuites");
  if (pRoot == nullptr) {
    return false;
  }

  tinyxml2::XMLElement* p = pRoot->FirstChildElement("testsuite");
  if (p == nullptr) {
    return false;
  }

  for (; p != nullptr; p = p->NextSiblingElement("testsuite")) {
    tinyxml2::XMLElement* q = p->FirstChildElement("testcase");
    if (q == nullptr) {
      return false;
    }

    for (; q != nullptr; q = q->NextSiblingElement("testcase")) {
      const char* pStatus = q->Attribute("status");
      if (pStatus == nullptr) {
        return false;
      }

      if (std::string_view(pStatus) == "run") {
        const char* pClassName = q->Attribute("classname");
        const char* pName = q->Attribute("name");
        if (pClassName == nullptr || pName == nullptr) {
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
