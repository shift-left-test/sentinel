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
  mBuildFailure(parser, "build_failure",
    "If build_failure occurs", {"build-failure"}) {
}

int CommandEvaluate::run() {
  namespace fs = std::experimental::filesystem;
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  fs::path outputDir = fs::canonical(mOutputDir.Get());

  auto logger = Logger::getLogger(cCommandEvaluateLoggerName);

  sentinel::Mutant m;
  std::istringstream iss(mMutantStr.Get());
  iss >> m;

  if (mIsVerbose.Get()) {
    logger->info(fmt::format("mutant: {}", mMutantStr));
  }

  sentinel::Evaluator evaluator(mExpectedDir.Get(), sourceRoot);

  evaluator.compareAndSaveMutationResult(m, mActualDir.Get(),
    outputDir / mEvalFile.Get(), static_cast<bool>(mBuildFailure));

  return 0;
}
}  // namespace sentinel
