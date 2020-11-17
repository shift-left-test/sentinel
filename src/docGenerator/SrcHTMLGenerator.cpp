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
#include <string>
#include "sentinel/docGenerator/SrcHTMLGenerator.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {
SrcHTMLGenerator::SrcHTMLGenerator(const std::string& srcName, bool srcRoot) :
    mSrcName(srcName), mSrcRoot(srcRoot) {
}

void SrcHTMLGenerator::pushLine(std::size_t curLineNum,
                                const std::string& curClass,
                                std::size_t numCurLineMrs,
                                const std::string& curCode,
                                const std::vector<
                                std::tuple<int, std::string, bool>>& explain) {
  std::string lineExplain;
  for (const auto& curExplain : explain) {
    lineExplain += fmt::format(lineExplainContent,
                fmt::arg("count", std::get<0>(curExplain)),
                fmt::arg("operator", std::get<1>(curExplain)),
                fmt::arg("killed_or_not",
                         std::get<2>(curExplain) ?
                         "KILLED" : "NO_COVERAGE"));
  }
  mLines += fmt::format(lineContent,
              fmt::arg("cur_lineNum", curLineNum),
              fmt::arg("src_name", mSrcName),
              fmt::arg("cur_class", curClass),
              fmt::arg("num_cur_line_mrs", numCurLineMrs == 0 ?
                                           "" : std::to_string(numCurLineMrs)),
              fmt::arg("line_explain", lineExplain),
              fmt::arg("cur_code", curCode));
}

void SrcHTMLGenerator::pushMutation(std::size_t curLineNum,
                                    bool killed,
                                    std::size_t count,
                                    const std::string& curKillingTest,
                                    const std::string& curOperator) {
  mMutations += fmt::format(mutationsContent,
                            fmt::arg("cur_lineNum", curLineNum),
                            fmt::arg("src_name", mSrcName),
                            fmt::arg("killed_or_not", killed ?
                                     "KILLED" : "NO_COVERAGE"),
                            fmt::arg("count", count),
                            fmt::arg("cur_killing_test",
                                     curKillingTest.empty() ?
                                     "none" : curKillingTest),
                            fmt::arg("operator", curOperator));
}

void SrcHTMLGenerator::pushMutator(const std::string& mutator) {
    mMutators += fmt::format(mutatorListContent,
                                fmt::arg("mutator", mutator));
}

void SrcHTMLGenerator::pushKillingTest(const std::string& killingTest) {
    mTestList += fmt::format(testListContent,
                            fmt::arg("test_function", killingTest));
}

std::string SrcHTMLGenerator::str() {
  std::string testListGuard;
  if (!mTestList.empty()) {
    testListGuard = fmt::format(testListGuardContent,
                                fmt::arg("test_list", mTestList));
  } else {
    testListGuard = testListEmptyContent;
  }

  return fmt::format(srcHtmlSkeleton,
                     fmt::arg("style", mSrcRoot ? "" : "../"),
                     fmt::arg("src_name", mSrcName),
                     fmt::arg("lines", mLines),
                     fmt::arg("mutations", mMutations),
                     fmt::arg("mutator_list", mMutators),
                     fmt::arg("test_list_guard", testListGuard),
                     fmt::arg("http", "http://"));
}

}  // namespace sentinel
