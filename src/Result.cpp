/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <tinyxml2/tinyxml2.h>
#include <experimental/filesystem>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/CTestXmlParser.hpp"
#include "sentinel/GoogleTestXmlParser.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/QTestXmlParser.hpp"
#include "sentinel/Result.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

Result::Result(const std::string& path) : mLogger(Logger::getLogger("Result")) {
  auto parser1 = std::make_shared<GoogleTestXmlParser>();
  auto parser2 = std::make_shared<CTestXmlParser>();
  auto parser3 = std::make_shared<QTestXmlParser>();
  parser1->setNext(parser2)->setNext(parser3);

  for (const auto& dirent : fs::recursive_directory_iterator(path)) {
    const auto& curPath = dirent.path();
    std::string curExt = curPath.extension().string();
    std::transform(curExt.begin(), curExt.end(), curExt.begin(), [](unsigned char c) { return std::tolower(c); });
    if (fs::is_regular_file(curPath) && curExt == ".xml") {
      parser1->process(curPath.string(), &mPassedTC, &mFailedTC);
    }
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
