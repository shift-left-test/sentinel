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
#include <fstream>
#include <iostream>
#include <memory>
#include "sentinel/Evaluator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/os.hpp"


namespace sentinel {

Evaluator::Evaluator(const Mutant& mut,
                     const std::string& expectedResultDir,
                     const std::string& outDir) :
    mMutant(mut), mExpectedResult(expectedResultDir), mOutDir(outDir),
    mLogger(Logger::getLogger("Evaluator")) {
  mLogger->debug(fmt::format("Load Expected Result: {}", expectedResultDir));

  if (os::path::exists(mOutDir)) {
    if (!os::path::isDirectory(mOutDir)) {
      throw InvalidArgumentException(fmt::format(
          "dirPath isn't directory({0})", mOutDir));
    }
  } else {
    os::createDirectories(mOutDir);
  }
}

MutationResult Evaluator::compareAndSaveMutationResult(
    const std::string& ActualResultDir) {
  Result mActualResult(ActualResultDir);
  mLogger->debug(fmt::format("Load Actual Result: {}", ActualResultDir));

  std::string killingTC = Result::kill(mExpectedResult, mActualResult);
  mLogger->debug(fmt::format("killing TC: {}", killingTC));

  std::cout << fmt::format(
      "{mu} ({path}, {sl}:{sc}-{el}:{ec}) {status}",
      fmt::arg("mu", mMutant.getOperator()),
      fmt::arg("path", os::path::filename(mMutant.getPath())),
      fmt::arg("sl", mMutant.getFirst().line),
      fmt::arg("sc", mMutant.getFirst().column),
      fmt::arg("el", mMutant.getLast().line),
      fmt::arg("ec", mMutant.getLast().column),
      fmt::arg("status", killingTC.length() != 0 ? "Killed" : "Survived"))
            << std::endl;

  MutationResult ret(mMutant, killingTC,
                     killingTC.length() != 0);

  std::string filePath = os::path::join(mOutDir, "MutationResult");

  std::ofstream outFile(filePath.c_str(),
      std::ios::out | std::ios::app);
  outFile << ret << std::endl;
  outFile.close();

  mLogger->debug(fmt::format("Save MutationResult: {}", mOutDir));

  return ret;
}

}  // namespace sentinel
