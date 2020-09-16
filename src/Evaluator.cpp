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

#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"


namespace sentinel {

Evaluator::Evaluator(const std::string& mutableDBDir,
    const std::string& expectedResultDir, const std::string& outDir,
    const std::shared_ptr<Logger>& logger)
    : mMutables(mutableDBDir), mExpectedResult(expectedResultDir),
    mLogger(logger), mOutDir(outDir) {
  mLogger->debug(
      std::string("Load Expected Result: ").append(expectedResultDir));
      mMutables.load();
  mLogger->debug(std::string("Load mutable DB: ").append(mutableDBDir));
}

MutationResult Evaluator::compareAndSaveMutationResult(
    const std::string& ActualResultDir, int mutableDBIdx) {
  auto mMutable = mMutables.get(mutableDBIdx);
  mLogger->debug(std::string("Load mutable idx: ").append(
      std::to_string(mutableDBIdx)));

  Result mActualResult(ActualResultDir);
  mLogger->debug(std::string("Load Actual Result: ").append(ActualResultDir));

  std::string killingTC = Result::kill(mExpectedResult, mActualResult);
  mLogger->debug(std::string("killing TC: ").append(killingTC));

  MutationResult ret(mMutable, killingTC,
      killingTC.length() != 0, mutableDBIdx);
  ret.saveToFile(mOutDir);
  mLogger->debug(std::string("Save MutationResult: ").append(mOutDir));

  return ret;
}

}  // namespace sentinel
