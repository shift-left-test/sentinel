/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <tinyxml2/tinyxml2.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "sentinel/CTestXmlParser.hpp"
#include "sentinel/GoogleTestXmlParser.hpp"
#include "sentinel/QTestXmlParser.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/io.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Result::Result(const std::string& path) {
  auto parser1 = std::make_shared<GoogleTestXmlParser>();
  auto parser2 = std::make_shared<CTestXmlParser>();
  auto parser3 = std::make_shared<QTestXmlParser>();
  parser1->setNext(parser2)->setNext(parser3);

  for (const auto& dirent : fs::recursive_directory_iterator(path)) {
    if (dirent.is_regular_file() && io::isXmlFile(dirent.path())) {
      parser1->process(dirent.path().string(), &mPassedTC, &mFailedTC);
    }
  }
  std::sort(mPassedTC.begin(), mPassedTC.end());
  std::sort(mFailedTC.begin(), mFailedTC.end());
}

bool Result::checkPassedTCEmpty() const {
  return mPassedTC.empty();
}

MutationState Result::compare(const Result& original, const Result& mutated, std::string* killingTest,
                              std::string* errorTest) {
  const std::unordered_set<std::string> mutatedPassed(mutated.mPassedTC.begin(), mutated.mPassedTC.end());
  const std::unordered_set<std::string> mutatedFailed(mutated.mFailedTC.begin(), mutated.mFailedTC.end());
  std::vector<std::string> killingTests;
  std::vector<std::string> errorTests;
  for (const std::string& tc : original.mPassedTC) {
    if (mutatedPassed.count(tc) == 0) {
      if (mutatedFailed.count(tc) == 0) {
        errorTests.push_back(tc);
      } else {
        killingTests.push_back(tc);
      }
    }
  }
  *killingTest = fmt::format("{}", fmt::join(killingTests, ", "));
  *errorTest = fmt::format("{}", fmt::join(errorTests, ", "));
  if (!killingTests.empty()) {
    return MutationState::KILLED;
  }
  if (!errorTests.empty()) {
    return MutationState::RUNTIME_ERROR;
  }
  return MutationState::SURVIVED;
}

}  // namespace sentinel
