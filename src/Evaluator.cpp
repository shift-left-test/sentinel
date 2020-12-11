/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

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
#include <fstream>
#include <iostream>
#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

Evaluator::Evaluator(const std::string& expectedResultDir,
    const std::string& sourcePath) : mSourcePath(sourcePath),
    mExpectedResult(expectedResultDir),
    mLogger(Logger::getLogger("Evaluator")) {
  mLogger->info(fmt::format("Load Expected Result: {}", expectedResultDir));
  auto checkZero = mExpectedResult.checkPassedTCEmpty();
  if (checkZero) {
    throw InvalidArgumentException(fmt::format(
        "No passed TC in Expected Result({0})", expectedResultDir));
  }
}

MutationResult Evaluator::compare(const Mutant& mut,
    const std::string& ActualResultDir, const std::string& testState) {
  std::string killingTC;
  std::string errorTC;
  MutationState state;

  if (testState == "build_failure") {
    state = MutationState::BUILD_FAILURE;
    mLogger->info(fmt::format("Build failure - ignore({})", ActualResultDir));
  } else if (testState == "timeout") {
    state = MutationState::TIMEOUT;
    mLogger->info(fmt::format("Timeout - ignore({})", ActualResultDir));
  } else if (testState == "success") {
    Result mActualResult(ActualResultDir);
    mLogger->info(fmt::format("Load Actual Result: {}", ActualResultDir));
    state = Result::compare(mExpectedResult, mActualResult,
        &killingTC, &errorTC);
  } else {
      throw InvalidArgumentException(fmt::format(
          "Invalid value for testState : {0}", testState));
  }
  mLogger->info(fmt::format("Mutant: {}", mut.str()));
  mLogger->info(fmt::format("killing TC: {}", killingTC));
  mLogger->info(fmt::format("error TC: {}", errorTC));
  mLogger->info(fmt::format("Mutation State: {}", MutationStateToStr(state)));

  fs::path relPath;

  auto p = fs::canonical(mut.getPath());
  auto base = fs::canonical(mSourcePath);

  auto mismatched = std::mismatch(p.begin(), p.end(), base.begin(), base.end());

  if (mismatched.first == p.end() && mismatched.second == base.end()) {
    relPath /= ".";
  } else {
    auto it_p = mismatched.first;
    auto it_base = mismatched.second;

    for (; it_base != base.end(); ++it_base) {
      if (!it_base->empty()) {
        relPath /= "..";
      }
    }

    for (; it_p != p.end(); ++it_p) {
      relPath /= *it_p;
    }
  }

  std::size_t flen = 60;
  std::string mutLoc = fmt::format(
      "{path} ({sl}:{sc}-{el}:{ec} -> {mc})",
      fmt::arg("path", relPath.string()),
      fmt::arg("sl", mut.getFirst().line),
      fmt::arg("sc", mut.getFirst().column),
      fmt::arg("el", mut.getLast().line),
      fmt::arg("ec", mut.getLast().column),
      fmt::arg("mc", mut.getToken().empty() ? "DELETE STMT": mut.getToken()));

  int filePos = mutLoc.size() - flen;
  std::string skipStr;
  if (filePos < 0) {
    filePos = 0;
  } else if (filePos > 1) {
    filePos += 4;
    skipStr = "... ";
  }

  std::cout << fmt::format(
      "{mu:>5} : {loc:.<{flen}} {status}",
      fmt::arg("mu", mut.getOperator()),
      fmt::arg("loc", skipStr + mutLoc.substr(filePos)),
      fmt::arg("flen", flen),
      fmt::arg("status", MutationStateToStr(state)))
            << std::endl;

  MutationResult ret(mut, killingTC, errorTC, state);

  mMutationResults.push_back(ret);

  return ret;
}

MutationResult Evaluator::compareAndSaveMutationResult(const Mutant& mut,
    const std::experimental::filesystem::path& ActualResultDir,
    const std::experimental::filesystem::path& evalFilePath,
    const std::string& testState) {
  auto outDir = evalFilePath.parent_path();
  if (fs::exists(outDir)) {
    if (!fs::is_directory(outDir)) {
      throw InvalidArgumentException(fmt::format(
          "dirPath isn't directory({0})", outDir.string()));
    }
  } else {
    fs::create_directories(outDir);
  }

  MutationResult ret = compare(mut, ActualResultDir, testState);

  std::ofstream outFile(evalFilePath.c_str(),
      std::ios::out | std::ios::app);
  outFile << ret << std::endl;
  outFile.close();

  mLogger->info(fmt::format("Save MutationResult: {}", outDir.string()));

  return ret;
}

}  // namespace sentinel
