/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <string>
#include <string_view>
#include "sentinel/Logger.hpp"
#include "sentinel/QTestXmlParser.hpp"

namespace sentinel {

bool QTestXmlParser::parse(std::shared_ptr<tinyxml2::XMLDocument> document) {
  std::string message = "This file doesn't follow QTest result format";

  tinyxml2::XMLElement* p = document->FirstChildElement("testsuite");
  if (p == nullptr) {
    Logger::debug(message);
    return false;
  }

  tinyxml2::XMLElement* q = p->FirstChildElement("testcase");
  if (q == nullptr) {
    Logger::debug(message);
    return false;
  }

  for (; q != nullptr; q = q->NextSiblingElement("testcase")) {
    const char* pResult = q->Attribute("result");
    if (pResult == nullptr) {
      Logger::debug(message);
      return false;
    }

    const char* pClassName = p->Attribute("name");
    const char* pName = q->Attribute("name");
    if (pClassName == nullptr || pName == nullptr) {
      Logger::debug(message);
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
