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
#include <algorithm>
#include <iostream>
#include <map>
#include "sentinel/CommandPopulate.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"


namespace sentinel {
const char * cCommandPopulateLoggerName = "CommandPopulate";

CommandPopulate::CommandPopulate(args::Subparser& parser) : Command(parser),
  mBuildDir(parser, "build_dir",
    "Directory where compile_commands.json file exists.",
    {'b', "build-dir"}, "."),
  mScope(parser, "scope",
    "Diff scope, one of ['commit', 'all'].",
    {'s', "scope"}, "all"),
  mExtensions(parser, "ext",
    "Extentions of source file which could be mutated.",
    {'t', "extension"}, {"cxx", "hxx", "cpp", "hpp", "cc", "hh", "c", "h",
    "c++", "h++", "cu", "cuh"}),
  mExcludes(parser, "path",
    "exclude file or path",
    {'e', "exclude"}),
  mLimit(parser, "count",
    "Maximum generated mutable count.",
    {'l', "limit"}, 10),
  mMutableFilename(parser, "path",
    "Populated result file name which will be created at output-dir.",
    {"mutants-file-name"}, "mutables.db") {
}

int CommandPopulate::run() {
  namespace fs = std::experimental::filesystem;
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  fs::path outputDir = fs::canonical(mOutputDir.Get());

  auto logger = Logger::getLogger(cCommandPopulateLoggerName);
  if (mIsVerbose.Get()) {
    logger->info(fmt::format("source root:{}", sourceRoot.string()));
    for (auto&exclude : mExcludes.Get()) {
      logger->info(fmt::format("exclude:{}", exclude));
    }
    for (auto& extension : mExtensions.Get()) {
      logger->info(fmt::format("extension:{}", extension));
    }
    logger->info(fmt::format("limit:{}", mLimit.Get()));
  }
  auto repo = std::make_unique<sentinel::GitRepository>(
    sourceRoot, mExtensions.Get(), mExcludes.Get());
  sentinel::SourceLines sourceLines = repo->getSourceLines(mScope.Get());

  // Shuffle target lines as an attempt to reduce mutant selecting time.
  auto rng = std::default_random_engine{};
  std::shuffle(std::begin(sourceLines), std::end(sourceLines), rng);

  auto generator = std::make_shared<sentinel::UniformMutantGenerator>(
      mBuildDir.Get());

  sentinel::MutationFactory mutationFactory(generator);

  sentinel::Mutants mutants = mutationFactory.populate(sourceRoot,
                                           sourceLines,
                                           mLimit.Get());
  if (mIsVerbose) {
    for (auto& mutant : mutants) {
      std::stringstream buf;
      buf << mutant;
      logger->info(fmt::format("mutant: {}", buf.str()));
    }
  }

  mutants.save(outputDir / mMutableFilename.Get());

  return 0;
}

}  // namespace sentinel
