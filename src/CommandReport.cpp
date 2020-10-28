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
#include <string>
#include "sentinel/HTMLReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/CommandReport.hpp"


namespace sentinel {
const char * cCommandReportLoggerName = "CommandReport";

CommandReport::CommandReport(CLI::App* app) {
  mSubApp = app->add_subcommand("report",
    "Create a mutation test report based on the'evaluate' result "
    "and source code");
  mSubApp->add_option("--evaluation-file", mEvalFile,
    "Mutation test result file")->required();
}

int CommandReport::run(const fs::path& sourceRoot,
  const fs::path& workDir, const fs::path& outputDir,
  bool verbose) {
  auto logger = Logger::getLogger(cCommandReportLoggerName);

  logger->info(fmt::format("evaluation-file: {}", mEvalFile));
  logger->info(fmt::format("output dir: {}", outputDir.string()));

  sentinel::XMLReport xmlReport(mEvalFile, sourceRoot);
  xmlReport.save(outputDir);
  sentinel::HTMLReport htmlReport(mEvalFile, sourceRoot);
  htmlReport.save(outputDir);
  htmlReport.printSummary();

  return 0;
}
}  // namespace sentinel
