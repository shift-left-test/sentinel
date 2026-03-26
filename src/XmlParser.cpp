/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <tinyxml2/tinyxml2.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/XmlParser.hpp"

namespace sentinel {

XmlParser::XmlParser() : mNext(nullptr) {
  reset();
}

std::shared_ptr<XmlParser> XmlParser::setNext(std::shared_ptr<XmlParser> parser) {
  mNext = parser;
  return mNext;
}

void XmlParser::reset() {
  mPassed = std::vector<std::string>();
  mFailed = std::vector<std::string>();
}

void XmlParser::addPassed(const std::string& name) {
  mPassed.push_back(name);
}

void XmlParser::addFailed(const std::string& name) {
  mFailed.push_back(name);
}

void XmlParser::collect(std::vector<std::string>* passed, std::vector<std::string>* failed) {
  passed->insert(passed->end(), mPassed.begin(), mPassed.end());
  failed->insert(failed->end(), mFailed.begin(), mFailed.end());
  std::sort(passed->begin(), passed->end());
  std::sort(failed->begin(), failed->end());
}

void XmlParser::process(const std::string& path, std::vector<std::string>* passed, std::vector<std::string>* failed) {
  auto document = std::make_shared<tinyxml2::XMLDocument>();
  document->LoadFile(path.c_str());
  reset();
  if (parse(document)) {
    collect(passed, failed);
    return;
  }
  if (mNext) {
    mNext->process(path, passed, failed);
  }
}

}  // namespace sentinel
