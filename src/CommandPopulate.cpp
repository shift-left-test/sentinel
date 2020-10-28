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
#include <algorithm>
#include <iostream>
#include <map>
#include "sentinel/CommandPopulate.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"


namespace sentinel {
const char * cCommandPopulateLoggerName = "CommandPopulate";

CommandPopulate::CommandPopulate(CLI::App* app) :
  mBuildDir("."),
  mScope("all"),
  mExtensions{"cxx", "hxx", "cpp", "hpp", "cc", "hh", "c", "h", "c++", "h++",
    "cu", "cuh"},
  mLimit(10),
  mMutableFilename("mutables.db") {
  mSubApp = app->add_subcommand("populate",
    "Identify mutable test targets and application methods in'git' "
    "and print a list");
  mSubApp->add_option("-b,--build-dir", mBuildDir,
    "Directory where compile_commands.json file exists.", true);
  mSubApp->add_option("-s,--scope", mScope,
    "Diff scope, one of ['commit', 'all'].", true);
  mSubApp->add_option("-t,--extension", mExtensions,
    "Extentions of source file which could be mutated.", true);
  mSubApp->add_option("-e,--exclude", mExcludes,
    "exclude file or path");
  mSubApp->add_option("-l,--limit", mLimit,
    "Maximum generated mutable count.", true);
  mSubApp->add_option("--mutants-file-name", mMutableFilename,
    "Populated result file name which will be created at output-dir.", true);
}

int CommandPopulate::run(const fs::path& sourceRoot,
  const fs::path& workDir, const fs::path& outputDir,
  bool verbose) {
  auto logger = Logger::getLogger(cCommandPopulateLoggerName);
  if (verbose) {
    logger->info(fmt::format("source root:{}", sourceRoot.string()));
    for (auto&exclude : mExcludes) {
      logger->info(fmt::format("exclude:{}", exclude));
    }
    for (auto& extension : mExtensions) {
      logger->info(fmt::format("extension:{}", extension));
    }
    logger->info(fmt::format("limit:{}", mLimit));
  }
  auto repo = std::make_unique<sentinel::GitRepository>(
    sourceRoot, mExtensions, mExcludes);
  sentinel::SourceLines sourceLines = repo->getSourceLines(mScope);

  // Shuffle target lines as an attempt to reduce mutant selecting time.
  auto rng = std::default_random_engine{};
  std::shuffle(std::begin(sourceLines), std::end(sourceLines), rng);

  auto generator = std::make_shared<sentinel::UniformMutantGenerator>(
      mBuildDir);

  sentinel::MutationFactory mutationFactory(generator);

  sentinel::Mutants mutants = mutationFactory.populate(sourceRoot,
                                           sourceLines,
                                           mLimit);
  if (verbose) {
    for (auto& mutant : mutants) {
      std::stringstream buf;
      buf << mutant;
      logger->info(fmt::format("mutant: {}", buf.str()));
    }
  }

  mutants.save(outputDir /mMutableFilename);

  return 0;
}

}  // namespace sentinel
