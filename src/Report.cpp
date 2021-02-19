/*
  MIT License

  Copyright (c) 2020 Sangmo Kang

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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

Report::Report(const MutationResults& results,
    const std::experimental::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mResults(results),
    mLogger(Logger::getLogger("Report")) {
  generateReport();
}

Report::Report(const std::experimental::filesystem::path& resultsPath,
    const std::experimental::filesystem::path& sourcePath) :
    mSourcePath(sourcePath), mLogger(Logger::getLogger("Report")) {
  mResults.load(resultsPath);
  mLogger->info(fmt::format("Load MutationResults: {}", resultsPath.string()));

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
    throw InvalidArgumentException(fmt::format("sourcePath doesn't exist({0})",
                                               mSourcePath.string()));
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

    fs::path mrPath = getRelativePath(mr.getMutant().getPath(),
                                            mSourcePath);
    std::string curDirname = mrPath.parent_path();
    curDirname = string::replaceAll(curDirname, "/", ".");

    if (groupByDirPath.empty() || groupByDirPath.count(curDirname) == 0) {
      groupByDirPath.emplace(curDirname,
          new std::tuple<std::vector<const MutationResult*>*, std::size_t,
                         std::size_t, std::size_t>(
          new std::vector<const MutationResult*>(), 0, 0, 0));
    }
    std::get<0>(*groupByDirPath[curDirname])->push_back(&mr);
    std::get<1>(*groupByDirPath[curDirname]) += 1;

    if (groupByPath.empty() ||
        groupByPath.count(mrPath) == 0) {
      groupByPath.emplace(mrPath,
          new std::tuple<std::vector<const MutationResult*>*, std::size_t,
                         std::size_t>(
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
  std::size_t mlen = 10;
  std::size_t klen = 10;
  std::size_t clen = 10;
  std::size_t maxlen = flen + mlen + klen + clen + 2;

  int cnt = 0;
  mLogger->info(fmt::format("# of MutationResults: {}", mResults.size()));
  for (const MutationResult& mr : mResults) {
    mLogger->info(fmt::format("MutationResult #{}", ++cnt));
    mLogger->info(fmt::format("  Mutant: {}", mr.getMutant().str()));
    mLogger->info(fmt::format("  killing TC: {}", mr.getKillingTest()));
    mLogger->info(fmt::format("  error TC: {}", mr.getErrorTest()));
    mLogger->info(fmt::format("  Mutation State: {}",
          MutationStateToStr(mr.getMutationState())));
  }

  std::string defFormat = "{0:<{1}}{2:>{3}}{4:>{5}}{6:>{7}}\n";
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << string::rtrim(fmt::format("{0:^{1}}",
                                         "Mutation Coverage Report", maxlen))
            << std::endl;
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat,
                           "File", flen,
                           "#killed", klen,
                           "#mutation", mlen,
                           "cov", clen);

  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  for (const auto& p : groupByPath) {
    int curCov = -1;
    if (std::get<1>(*p.second) != 0) {
      curCov = 100 * std::get<2>(*p.second) / std::get<1>(*p.second);
    }
    int filePos = p.first.string().size() - flen;
    std::string skipStr;
    if (filePos < 0) {
      filePos = 0;
    } else if (filePos > 1) {
      filePos += 4;
      skipStr = "... ";
    }
    std::cout << fmt::format(defFormat,
                             skipStr + p.first.string().substr(filePos), flen,
                             std::get<2>(*p.second), klen,
                             std::get<1>(*p.second), mlen,
                             (curCov != -1 ?
                             std::to_string(curCov) : std::string("-")) + "%",
                             clen);
  }
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  int finalCov = -1;
  if (totNumberOfMutation != 0) {
    finalCov = 100 * totNumberOfDetectedMutation / totNumberOfMutation;
  }
  std::cout << fmt::format(defFormat,
                           "TOTAL", flen,
                           totNumberOfDetectedMutation, klen,
                           totNumberOfMutation, mlen,
                           (finalCov != -1 ?
                           std::to_string(finalCov) : std::string("-")) + "%",
                           clen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  if ((totNumberOfBuildFailure + totNumberOfRuntimeError +
        totNumberOfTimeout) != 0) {
    std::cout << fmt::format("Ignored Mutation\n");
    std::cout << string::rtrim(fmt::format(defFormat,
                                           "Build Failure", flen,
                                            "", klen,
                                            totNumberOfBuildFailure, mlen,
                                            "", clen))
              << std::endl;
    std::cout << string::rtrim(fmt::format(defFormat,
                                           "Runtime Error", flen,
                                           "", klen,
                                           totNumberOfRuntimeError, mlen,
                                           "", clen))
              << std::endl;
    std::cout << string::rtrim(fmt::format(defFormat,
                                           "Timeout", flen,
                                           "", klen,
                                           totNumberOfTimeout, mlen,
                                           "", clen))
              << std::endl;
    std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  }
}

fs::path Report::getRelativePath(
    const std::string& path, const std::string& start) {
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
