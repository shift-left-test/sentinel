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
#include <string>
#include "sentinel/HTMLReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/CommandReport.hpp"


namespace sentinel {
const char * cCommandReportLoggerName = "CommandReport";

CommandReport::CommandReport(args::Subparser& parser) : Command(parser),
  mEvalFile(parser, "PATH",
    "Mutation test result file.",
    {"evaluation-file"}, args::Options::Required),
  mWeakMutation(parser, "weak_mutation",
      R"(If weak-mutation flag is on, regard runtime errors during test as detected mutation)",
      {"weak-mutation"}) {
}

int CommandReport::run() {
  namespace fs = std::experimental::filesystem;
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  fs::path outputDir = fs::canonical(mOutputDir.Get());

  auto logger = Logger::getLogger(cCommandReportLoggerName);

  logger->info(fmt::format("evaluation-file: {}", mEvalFile.Get()));
  logger->info(fmt::format("output dir: {}", outputDir.string()));

  sentinel::XMLReport xmlReport(mEvalFile.Get(), sourceRoot,
      !static_cast<bool>(mWeakMutation));
  xmlReport.save(outputDir);
  sentinel::HTMLReport htmlReport(mEvalFile.Get(), sourceRoot,
      !static_cast<bool>(mWeakMutation));
  htmlReport.save(outputDir);
  htmlReport.printSummary();

  return 0;
}
}  // namespace sentinel
