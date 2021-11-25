/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
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
    {"evaluation-file"}, args::Options::Required) {
}

int CommandReport::run() {
  namespace fs = std::experimental::filesystem;
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());

  auto logger = Logger::getLogger(cCommandReportLoggerName);

  logger->info(fmt::format("evaluation-file: {}", mEvalFile.Get()));

  if (mOutputDir.Get().empty()) {
    logger->info("Output dir is not given. Pass generating Report file.");
    sentinel::XMLReport xmlReport(mEvalFile.Get(), sourceRoot);
    xmlReport.printSummary();
  } else {
    fs::create_directories(mOutputDir.Get());
    fs::path outputDir = fs::canonical(mOutputDir.Get());
    logger->info(fmt::format("output dir: {}", outputDir.string()));
    sentinel::XMLReport xmlReport(mEvalFile.Get(), sourceRoot);
    xmlReport.save(outputDir);
    sentinel::HTMLReport htmlReport(mEvalFile.Get(), sourceRoot);
    htmlReport.save(outputDir);
    htmlReport.printSummary();
  }
  return 0;
}
}  // namespace sentinel
