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
#include <iostream>
#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"


namespace sentinel {

Evaluator::Evaluator(const std::string& mutableDBDir,
                     const std::string& expectedResultDir,
                     const std::string& outDir,
                     const std::shared_ptr<Logger>& logger) :
    mExpectedResult(expectedResultDir), mLogger(logger), mOutDir(outDir) {
  if (mLogger == nullptr) {
    mLogger = Logger::getLogger("Evaluator");
  }
  mLogger->debug(fmt::format("Load Expected Result: {}", expectedResultDir));
  mMutables.load(mutableDBDir);
  mLogger->debug(fmt::format("Load mutable DB: {}", mutableDBDir));
}

MutationResult Evaluator::compareAndSaveMutationResult(
    const std::string& ActualResultDir, int mutableDBIdx) {
  auto mMutable = mMutables.at(mutableDBIdx);
  mLogger->debug(fmt::format("Load mutable idx: {}", mutableDBIdx));

  Result mActualResult(ActualResultDir);
  mLogger->debug(fmt::format("Load Actual Result: {}", ActualResultDir));

  std::string killingTC = Result::kill(mExpectedResult, mActualResult);
  mLogger->debug(fmt::format("killing TC: {}", killingTC));

  std::cout << fmt::format(
      "{idx:>5}/{size:<5}: {mu}({path}, {sl}:{sc}-{el}:{ec}) {status}{kT}",
      fmt::arg("idx", mutableDBIdx+1),
      fmt::arg("size", mMutables.size()),
      fmt::arg("mu", mMutable.getOperator()),
      fmt::arg("path", mMutable.getPath()),
      fmt::arg("sl", mMutable.getFirst().line),
      fmt::arg("sc", mMutable.getFirst().column),
      fmt::arg("el", mMutable.getLast().line),
      fmt::arg("ec", mMutable.getLast().column),
      fmt::arg("status", killingTC.length() != 0 ? "Killed" : "Survived"),
      fmt::arg("kT", killingTC.length() != 0 ? " by " + killingTC  : ""))
            << std::endl;

  MutationResult ret(mMutable, killingTC,
                     killingTC.length() != 0, mutableDBIdx);
  ret.saveToFile(mOutDir);
  mLogger->debug(fmt::format("Save MutationResult: {}", mOutDir));

  return ret;
}

}  // namespace sentinel
