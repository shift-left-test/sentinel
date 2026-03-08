/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

Report::Report(const MutationResults& results, const std::experimental::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mResults(results), mLogger(Logger::getLogger("Report")) {
  generateReport();
}

Report::Report(const std::experimental::filesystem::path& resultsPath,
               const std::experimental::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mLogger(Logger::getLogger("Report")) {
  mResults.load(resultsPath);
  mLogger->verbose(fmt::format("Load MutationResults: {}", resultsPath.string()));

  generateReport();
}

Report::~Report() {
  for (const auto& p : groupByDirPath) {
    delete std::get<0>(*p.second);
    delete p.second;
  }

  for (const auto& p : groupByPath) {
    delete std::get<0>(*p.second);
    delete p.second;
  }
}

void Report::generateReport() {
  namespace fs = std::experimental::filesystem;
  if (!fs::exists(mSourcePath) || !fs::is_directory(mSourcePath)) {
    throw InvalidArgumentException(fmt::format("source path does not exist: {}", mSourcePath.string()));
  }

  for (const MutationResult& mr : mResults) {
    auto currentState = mr.getMutationState();
    if (currentState == MutationState::BUILD_FAILURE) {
      totNumberOfBuildFailure++;
      continue;
    }
    if (currentState == MutationState::RUNTIME_ERROR) {
      totNumberOfRuntimeError++;
      continue;
    }
    if (currentState == MutationState::TIMEOUT) {
      totNumberOfTimeout++;
      continue;
    }
    totNumberOfMutation++;

    fs::path mrPath = getRelativePath(mr.getMutant().getPath(), mSourcePath);
    std::string curDirname = mrPath.parent_path();
    curDirname = string::replaceAll(curDirname, "/", ".");

    if (groupByDirPath.empty() || groupByDirPath.count(curDirname) == 0) {
      groupByDirPath.emplace(curDirname,
                             new std::tuple<std::vector<const MutationResult*>*, std::size_t, std::size_t, std::size_t>(
                                 new std::vector<const MutationResult*>(), 0, 0, 0));
    }
    std::get<0>(*groupByDirPath[curDirname])->push_back(&mr);
    std::get<1>(*groupByDirPath[curDirname]) += 1;

    if (groupByPath.empty() || groupByPath.count(mrPath) == 0) {
      groupByPath.emplace(mrPath, new std::tuple<std::vector<const MutationResult*>*, std::size_t, std::size_t>(
                                      new std::vector<const MutationResult*>(), 0, 0));
    }
    std::get<0>(*groupByPath[mrPath])->push_back(&mr);
    std::get<1>(*groupByPath[mrPath]) += 1;

    if (mr.getDetected()) {
      std::get<2>(*groupByDirPath[curDirname]) += 1;
      std::get<2>(*groupByPath[mrPath]) += 1;
      totNumberOfDetectedMutation += 1;
    }
  }

  for (const auto& p : groupByDirPath) {
    std::set<std::string> tmpSet;
    for (const MutationResult* mr : *(std::get<0>(*p.second))) {
      tmpSet.insert(mr->getMutant().getPath());
    }
    std::get<3>(*p.second) = tmpSet.size();
  }
}

void Report::printSummary() {
  std::size_t flen = 50;
  std::size_t klen = 10;
  std::size_t mlen = 10;
  std::size_t clen = 10;
  std::size_t maxlen = flen + klen + mlen + clen + 2;

  int cnt = 0;
  mLogger->verbose(fmt::format("# of MutationResults: {}", mResults.size()));
  for (const MutationResult& mr : mResults) {
    mLogger->verbose(fmt::format("MutationResult #{}", ++cnt));
    mLogger->verbose(fmt::format("  Mutant: {}", mr.getMutant().str()));
    mLogger->verbose(fmt::format("  killing TC: {}", mr.getKillingTest()));
    mLogger->verbose(fmt::format("  error TC: {}", mr.getErrorTest()));
    mLogger->verbose(fmt::format("  Mutation State: {}", MutationStateToStr(mr.getMutationState())));
  }

  std::string defFormat = "{0:<{1}}{2:>{3}}{4:>{5}}{6:>{7}}\n";
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);
  std::cout << string::rtrim(fmt::format("{0:^{1}}", "Mutation Coverage Report", maxlen)) << "\n";
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat, "File", flen, "Killed", klen, "Total", mlen, "Score", clen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  for (const auto& p : groupByPath) {
    double curScore = -1.0;
    if (std::get<1>(*p.second) != 0) {
      curScore = (100.0 * std::get<2>(*p.second)) / std::get<1>(*p.second);
    }
    int filePos = p.first.string().size() - flen;
    std::string skipStr;
    if (filePos < 0) {
      filePos = 0;
    } else if (filePos > 1) {
      filePos += 4;
      skipStr = "... ";
    }
    std::string scoreStr = curScore >= 0.0 ? fmt::format("{:.1f}%", curScore) : "-%";
    std::cout << fmt::format(defFormat, skipStr + p.first.string().substr(filePos), flen,
                             std::get<2>(*p.second), klen, std::get<1>(*p.second), mlen,
                             scoreStr, clen);
  }
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  double finalScore = -1.0;
  if (totNumberOfMutation != 0) {
    finalScore = (100.0 * totNumberOfDetectedMutation) / totNumberOfMutation;
  }
  std::string finalScoreStr = finalScore >= 0.0 ? fmt::format("{:.1f}%", finalScore) : "-%";
  std::cout << fmt::format(defFormat, "TOTAL", flen, totNumberOfDetectedMutation, klen,
                           totNumberOfMutation, mlen, finalScoreStr, clen);
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);

  if ((totNumberOfBuildFailure + totNumberOfRuntimeError + totNumberOfTimeout) != 0) {
    std::string skipped;
    if (totNumberOfBuildFailure > 0) {
      skipped += fmt::format("{} build failure{}", totNumberOfBuildFailure,
                             totNumberOfBuildFailure == 1 ? "" : "s");
    }
    if (totNumberOfRuntimeError > 0) {
      if (!skipped.empty()) skipped += "  \xc2\xb7  ";
      skipped += fmt::format("{} runtime error{}", totNumberOfRuntimeError,
                             totNumberOfRuntimeError == 1 ? "" : "s");
    }
    if (totNumberOfTimeout > 0) {
      if (!skipped.empty()) skipped += "  \xc2\xb7  ";
      skipped += fmt::format("{} timeout{}", totNumberOfTimeout,
                             totNumberOfTimeout == 1 ? "" : "s");
    }
    std::cout << "Skipped: " << skipped << "\n";
    std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);
  }
}

fs::path Report::getRelativePath(const std::string& path, const std::string& start) {
  // 1. convert p and base to absolute paths
  auto p = fs::canonical(path);
  auto base = fs::canonical(start);

  // 2. find first mismatch and shared root path
  auto mismatched = std::mismatch(p.begin(), p.end(), base.begin(), base.end());

  // 3. if no mismatch return "."
  if (mismatched.first == p.end() && mismatched.second == base.end()) {
    return ".";
  }

  auto it_p = mismatched.first;
  auto it_base = mismatched.second;

  fs::path ret;

  // 4. iterate abase to the shared root and append "../"
  for (; it_base != base.end(); ++it_base) {
    if (!it_base->empty()) {
      ret /= "..";
    }
  }

  // 5. iterate from the shared root to the p and append its parts
  for (; it_p != p.end(); ++it_p) {
    ret /= *it_p;
  }
  return ret;
}

}  // namespace sentinel
