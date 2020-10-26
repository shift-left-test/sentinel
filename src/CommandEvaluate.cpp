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
#include <sstream>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/CommandEvaluate.hpp"


namespace sentinel {
const char * cCommandEvaluateLoggerName = "CommandEvaluate";

CommandEvaluate::CommandEvaluate(CLI::App* app) :
  mEvalFile("EvaluationResults") {
  mSubApp = app->add_subcommand("evaluate",
    "Compare the test result with mutable applied and the test result "
    "not applied");
  mSubApp->add_option("-m,--mutant", mMutantStr,
  "Mutant string")->required();
  mSubApp->add_option("-e,--expected", mExpectedDir,
    "Expected result directory")->required();
  mSubApp->add_option("-a,--actual", mActualDir,
    "Actual result directory")->required();
  mSubApp->add_option("--evaluation-file", mEvalFile,
    "Evaluated output filename(joined with output-dir)", true);
}

int CommandEvaluate::run(const std::string& sourceRoot,
  const std::string& workDir, const std::string& outputDir,
  bool verbose) {
  auto logger = Logger::getLogger(cCommandEvaluateLoggerName);
  sentinel::Mutant m;
  std::istringstream iss(mMutantStr);
  iss >> m;

  if (verbose) {
    logger->info(fmt::format("mutant: {}", mMutantStr));
  }

  sentinel::Evaluator evaluator(mExpectedDir, sourceRoot);

  evaluator.compareAndSaveMutationResult(m, mActualDir,
    sentinel::os::path::join(outputDir, mEvalFile));

  return 0;
}
}  // namespace sentinel
