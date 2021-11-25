/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
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
                                std::tuple<int, std::string, std::string,
                                std::string, bool>>& explain) {
  std::string lineExplain;
  for (const auto& curExplain : explain) {
    lineExplain += fmt::format(lineExplainContent,
                fmt::arg("count", std::get<0>(curExplain)),
                fmt::arg("operator", std::get<1>(curExplain)),
                fmt::arg("original_code", std::get<2>(curExplain)),
                fmt::arg("mutated_code", std::get<3>(curExplain)),
                fmt::arg("killed_or_not",
                         std::get<4>(curExplain) ?
                         "KILLED" : "SURVIVED"));
  }
  std::string rCode = string::replaceAll(curCode, "<", "&lt;");
  rCode = string::replaceAll(rCode, ">", "&gt;");
  mLines += fmt::format(lineContent,
              fmt::arg("cur_lineNum", curLineNum),
              fmt::arg("src_name", mSrcName),
              fmt::arg("cur_class", curClass),
              fmt::arg("num_cur_line_mrs", numCurLineMrs == 0 ?
                                           "" : std::to_string(numCurLineMrs)),
              fmt::arg("line_explain", lineExplain),
              fmt::arg("cur_code", rCode));
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
                                     "KILLED" : "SURVIVED"),
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
                     fmt::arg("https", "https://"));
}

}  // namespace sentinel
