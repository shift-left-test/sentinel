/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <string>
#include <string_view>
#include "sentinel/QTestXmlParser.hpp"

namespace sentinel {

bool QTestXmlParser::parse(std::shared_ptr<tinyxml2::XMLDocument> document) {
  tinyxml2::XMLElement* p = document->FirstChildElement("testsuite");
  if (p == nullptr) {
    return false;
  }

  tinyxml2::XMLElement* q = p->FirstChildElement("testcase");
  if (q == nullptr) {
    return false;
  }

  for (; q != nullptr; q = q->NextSiblingElement("testcase")) {
    const char* pResult = q->Attribute("result");
    if (pResult == nullptr) {
      return false;
    }

    const char* pClassName = p->Attribute("name");
    const char* pName = q->Attribute("name");
    if (pClassName == nullptr || pName == nullptr) {
      return false;
    }

    auto savedName = fmt::format("{0}.{1}", pClassName, pName);
    std::string_view res(pResult);
    if (res == "pass") {
      addPassed(savedName);
    } else if (res == "fail") {
      addFailed(savedName);
    } else {
      // do nothing
    }
  }

  return true;
}

}  // namespace sentinel
