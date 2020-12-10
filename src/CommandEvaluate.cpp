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
#include <iostream>
#include <sstream>
#include "sentinel/Evaluator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/CommandEvaluate.hpp"


namespace sentinel {
const char * cCommandEvaluateLoggerName = "CommandEvaluate";

CommandEvaluate::CommandEvaluate(args::Subparser& parser) : Command(parser),
  mMutantStr(parser, "mutant",
    "Mutant string",
    {'m', "mutant"}, args::Options::Required),
  mExpectedDir(parser, "PATH",
    "Expected result directory",
    {'e', "expected"}, args::Options::Required),
  mActualDir(parser, "PATH",
    "Actual result directory",
    {'a', "actual"}, args::Options::Required),
  mEvalFile(parser, "FILENAME",
    "Evaluated output file name which will be joined with output-dir.",
    {"evaluation-file"}, "EvaluationResults"),
  mTestState(parser, "TEST_STATE",
    R"asdf(Select the state of the test to be evaluated, one of ['success', 'build_failure', 'timeout'].)asdf",
    {"test-state"}, "success") {
}

int CommandEvaluate::run() {
  namespace fs = std::experimental::filesystem;

  if (mTestState.Get() != "build_failure" && mTestState.Get() != "timeout"
      && mTestState.Get() != "success") {
      throw InvalidArgumentException(fmt::format(
          "Invalid value for test-state option: {0}", mTestState.Get()));
  }

  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  std::string outputDirStr;
  if (mOutputDir.Get().empty()) {
    outputDirStr = ".";
  } else {
    outputDirStr = mOutputDir.Get();
  }
  fs::create_directories(outputDirStr);
  fs::path outputDir = fs::canonical(outputDirStr);

  auto logger = Logger::getLogger(cCommandEvaluateLoggerName);

  sentinel::Mutant m;
  std::istringstream iss(mMutantStr.Get());
  iss >> m;

  if (mIsVerbose.Get()) {
    logger->info(fmt::format("mutant: {}", mMutantStr));
  }

  sentinel::Evaluator evaluator(mExpectedDir.Get(), sourceRoot);

  evaluator.compareAndSaveMutationResult(m, mActualDir.Get(),
    outputDir / mEvalFile.Get(), mTestState.Get());

  return 0;
}
}  // namespace sentinel
