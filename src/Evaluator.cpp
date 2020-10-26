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

Evaluator::Evaluator(const std::string& expectedResultDir,
    const std::string& sourcePath) : mSourcePath(sourcePath),
    mExpectedResult(expectedResultDir),
    mLogger(Logger::getLogger("Evaluator")) {
  mLogger->debug(fmt::format("Load Expected Result: {}", expectedResultDir));
}

MutationResult Evaluator::compare(const Mutant& mut,
    const std::string& ActualResultDir) {
  Result mActualResult(ActualResultDir);
  mLogger->debug(fmt::format("Load Actual Result: {}", ActualResultDir));

  std::string killingTC = Result::kill(mExpectedResult, mActualResult);
  mLogger->debug(fmt::format("killing TC: {}", killingTC));

  std::size_t flen = 60;
  std::string mutLoc = fmt::format(
      "{path} ({sl}:{sc}-{el}:{ec})",
      fmt::arg("path", os::path::getRelativePath(
      mut.getPath(), mSourcePath)),
      fmt::arg("sl", mut.getFirst().line),
      fmt::arg("sc", mut.getFirst().column),
      fmt::arg("el", mut.getLast().line),
      fmt::arg("ec", mut.getLast().column));

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
      fmt::arg("status", killingTC.length() != 0 ? "Killed" : "Survived"))
            << std::endl;

  MutationResult ret(mut, killingTC,
                     killingTC.length() != 0);

  mMutationResults.push_back(ret);

  return ret;
}

MutationResult Evaluator::compareAndSaveMutationResult(const Mutant& mut,
    const std::string& ActualResultDir, const std::string& evalFilePath) {
  std::string outDir = os::path::dirname(evalFilePath);
  if (os::path::exists(outDir)) {
    if (!os::path::isDirectory(outDir)) {
      throw InvalidArgumentException(fmt::format(
          "dirPath isn't directory({0})", outDir));
    }
  } else {
    os::createDirectories(outDir);
  }

  MutationResult ret = compare(mut, ActualResultDir);

  std::ofstream outFile(evalFilePath.c_str(),
      std::ios::out | std::ios::app);
  outFile << ret << std::endl;
  outFile.close();

  mLogger->debug(fmt::format("Save MutationResult: {}", outDir));

  return ret;
}

}  // namespace sentinel
