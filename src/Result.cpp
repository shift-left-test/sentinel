/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <tinyxml2/tinyxml2.h>
#include <experimental/filesystem>
#include <algorithm>
#include <vector>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/Result.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

Result::Result(const std::string& path) :
    mLogger(Logger::getLogger("Result")) {
  std::vector<std::string> nonGtestFiles;
  std::string logFormat = "This file doesn't follow googletest result format: {}";

  // gtest result format
  for (const auto& dirent : fs::recursive_directory_iterator(path)) {
    const auto& curPath = dirent.path();
    std::string curExt = curPath.extension().string();
    std::transform(curExt.begin(), curExt.end(), curExt.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (fs::is_regular_file(curPath) && curExt == ".xml") {
      const auto& xmlPath = curPath.string();
      nonGtestFiles.push_back(xmlPath);
      std::vector<std::string> tmpPassedTC;
      std::vector<std::string> tmpFailedTC;
      tinyxml2::XMLDocument doc;
      auto errcode = doc.LoadFile(xmlPath.c_str());
      if (errcode != 0) {
        mLogger->debug(fmt::format("{}: {}", tinyxml2::XMLDocument::ErrorIDToName(errcode), xmlPath));
        continue;
      }

      tinyxml2::XMLElement *pRoot = doc.FirstChildElement("testsuites");
      if (pRoot == nullptr) {
        mLogger->debug(fmt::format(logFormat, xmlPath));
        continue;
      }
      tinyxml2::XMLElement *p = pRoot->FirstChildElement("testsuite");
      if (p == nullptr) {
        mLogger->debug(fmt::format(logFormat, xmlPath));
        continue;
      }

      bool allParsed = true;
      for ( ; p != nullptr && allParsed; p = p->NextSiblingElement("testsuite")) {
        tinyxml2::XMLElement *q = p->FirstChildElement("testcase");
        if (q == nullptr) {
          mLogger->debug(fmt::format(logFormat, xmlPath));
          allParsed = false;
          break;
        }
        for ( ; q != nullptr ; q = q->NextSiblingElement("testcase")) {
          const char* pStatus = q->Attribute("status");
          if (pStatus == nullptr) {
            mLogger->debug(fmt::format(logFormat, xmlPath));
            allParsed = false;
            break;
          }

          if (std::string(pStatus) == std::string("run")) {
            const char* pClassName = q->Attribute("classname");
            const char* pName = q->Attribute("name");
            if (pClassName == nullptr || pName == nullptr) {
              mLogger->debug(fmt::format(logFormat, xmlPath));
              allParsed = false;
              break;
            }

            auto savedName = fmt::format("{0}.{1}", pClassName, pName);

            if (q->FirstChildElement("failure") == nullptr) {
              tmpPassedTC.push_back(savedName);
            } else {
              tmpFailedTC.push_back(savedName);
            }
          }
        }
      }
      if (allParsed) {
        mPassedTC.insert(mPassedTC.end(), tmpPassedTC.begin(), tmpPassedTC.end());
        mFailedTC.insert(mFailedTC.end(), tmpFailedTC.begin(), tmpFailedTC.end());
      } else {
        nonGtestFiles.pop_back();
      }
    }
  }

  logFormat = "This file doesn't follow QtTest result format: {}";
  // QtTest result format
  for (const std::string& xmlPath : nonGtestFiles) {
    std::vector<std::string> tmpPassedTC;
    std::vector<std::string> tmpFailedTC;
    tinyxml2::XMLDocument doc;
    doc.LoadFile(xmlPath.c_str());

    tinyxml2::XMLElement *p = doc.FirstChildElement("testsuite");
    if (p == nullptr) {
      mLogger->debug(fmt::format(logFormat + "1", xmlPath));
      continue;
    }

    tinyxml2::XMLElement *q = p->FirstChildElement("testcase");
    if (q == nullptr) {
      mLogger->debug(fmt::format(logFormat + "1", xmlPath));
      continue;
    }

    bool allParsed = true;
    for ( ; q != nullptr ; q = q->NextSiblingElement("testcase")) {
      const char* pResult = q->Attribute("result");
      if (pResult == nullptr) {
        mLogger->debug(fmt::format(logFormat + "2", xmlPath));
        allParsed = false;
        break;
      }
      const char* pClassName = p->Attribute("name");
      const char* pName = q->Attribute("name");
      if (pClassName == nullptr || pName == nullptr) {
        mLogger->debug(fmt::format(logFormat + "3", xmlPath));
        allParsed = false;
        break;
      }

      auto savedName = fmt::format("{0}.{1}", pClassName, pName);

      if (std::string(pResult) == std::string("pass")) {
        tmpPassedTC.push_back(savedName);
      } else if (std::string(pResult) == std::string("fail")) {
        tmpFailedTC.push_back(savedName);
      }
    }
    if (allParsed) {
      mPassedTC.insert(mPassedTC.end(), tmpPassedTC.begin(), tmpPassedTC.end());
      mFailedTC.insert(mFailedTC.end(), tmpFailedTC.begin(), tmpFailedTC.end());
    }
  }

  if (!mPassedTC.empty()) {
    std::sort(mPassedTC.begin(), mPassedTC.end());
    std::sort(mFailedTC.begin(), mFailedTC.end());
  }
}

bool Result::checkPassedTCEmpty() {
  return mPassedTC.empty();
}

MutationState Result::compare(const Result& original, const Result& mutated,
                              std::string* killingTest, std::string* errorTest) {
  killingTest->clear();
  errorTest->clear();
  for (const std::string &tc : original.mPassedTC) {
    if (std::find(mutated.mPassedTC.begin(), mutated.mPassedTC.end(), tc) == mutated.mPassedTC.end()) {
      if (std::find(mutated.mFailedTC.begin(), mutated.mFailedTC.end(), tc) == mutated.mFailedTC.end()) {
        if (!errorTest->empty()) {
          errorTest->append(", ");
        }
        errorTest->append(tc);
      } else {
        if (!killingTest->empty()) {
          killingTest->append(", ");
        }
        killingTest->append(tc);
      }
    }
  }
  if (!errorTest->empty()) {
    return MutationState::RUNTIME_ERROR;
  }
  if (!killingTest->empty()) {
    return MutationState::KILLED;
  }
  return MutationState::SURVIVED;
}

}  // namespace sentinel
