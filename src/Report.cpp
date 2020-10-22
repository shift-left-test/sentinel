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
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

Report::Report(const MutationResults& results, const std::string& sourcePath) :
    mSourcePath(sourcePath), mResults(results) {
  generateReport();
}

Report::Report(const std::string& resultsPath, const std::string& sourcePath) :
    mSourcePath(sourcePath) {
  mResults.load(resultsPath);

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
  if (!os::path::exists(mSourcePath) || !os::path::isDirectory(mSourcePath)) {
    throw InvalidArgumentException(fmt::format("sourcePath doesn't exist({0})",
                                               mSourcePath));
  }

  totNumberOfMutation = mResults.size();

  for (const MutationResult& mr : mResults) {
    auto mrPath = os::path::getRelativePath(mr.getMutant().getPath(),
                                            mSourcePath);
    std::string curDirname = os::path::dirname(mrPath);
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
  std::string defFormat = "{0:<{1}}{2:>{3}}{4:>{5}}{6:>{7}}\n";
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format("{0:^{1}}\n", "Mutation Coverage Report", maxlen);
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
    int filePos = p.first.size() - flen;
    std::string skipStr;
    if (filePos < 0) {
      filePos = 0;
    } else if (filePos > 1) {
      filePos += 4;
      skipStr = "... ";
    }
    std::cout << fmt::format(defFormat,
                             skipStr + p.first.substr(filePos), flen,
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
}

}  // namespace sentinel
