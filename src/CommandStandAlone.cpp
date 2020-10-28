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
#include <cstdlib>
#include <map>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/CommandStandAlone.hpp"


namespace sentinel {
const char * cCommandStandAloneLoggerName = "CommandStandAlone";

CommandStandAlone::CommandStandAlone(CLI::App* app) :
  mBuildDir("."),
  mScope("all"),
  mExtensions{"cxx", "hxx", "cpp", "hpp", "cc", "hh", "c", "h", "c++", "h++",
    "cu", "cuh"},
  mLimit(10),
  mTestResultFileExts{"xml", "XML"} {
  mSubApp = app->add_subcommand("run",
    "Run mutation test in standalone mode");
  mSubApp->add_option("-b,--build-dir", mBuildDir,
    "Directory where compile_commands.json file exists.", true);
  mSubApp->add_option("-s,--scope", mScope,
    "diff scope, one of ['commit', 'all'].", true);
  mSubApp->add_option("-t,--extension", mExtensions,
    "Extentions of source file which could be mutated.", true);
  mSubApp->add_option("-e,--exclude", mExcludes,
    "exclude file or path");
  mSubApp->add_option("-l,--limit", mLimit,
    "Maximum generated mutable count.", true);
  mSubApp->add_option("--build-command", mBuildCmd,
    "Shell command to build source")->required();
  mSubApp->add_option("--test-command", mTestCmd,
    "Shell command to execute test")->required();
  mSubApp->add_option("--test-result-dir", mTestResultDir,
    "Test command output directory")->required();
  mSubApp->add_option("--test-result-extention", mTestResultFileExts,
    "Test command output file extensions", true);
}

int CommandStandAlone::run(const fs::path& sourceRoot,
  const fs::path& workDir, const fs::path& outputDir,
  bool verbose) {
  auto logger = Logger::getLogger(cCommandStandAloneLoggerName);
  logger->info(fmt::format("build dir: {}", mBuildDir));
  logger->info(fmt::format("build cmd: {}", mBuildCmd));
  logger->info(fmt::format("test cmd:{}", mTestCmd));
  logger->info(fmt::format("test result dir: {}", mTestResultDir));

  mBuildDir = fs::canonical(mBuildDir);
  fs::create_directories(mTestResultDir);
  mTestResultDir = fs::canonical(mTestResultDir);
  // create work directories
  fs::create_directories(workDir);
  fs::create_directories(workDir / "backup");
  fs::create_directories(workDir / "expected");
  fs::create_directories(workDir / "actual");

  std::string backupDir = fs::canonical(workDir / "backup");
  std::string expectedDir = fs::canonical(workDir / "expected");
  std::string actualDir = fs::canonical(workDir / "actual");

  // restore if previous backup is exists
  restoreBackup(backupDir, sourceRoot);

  ::chdir(mBuildDir.c_str());
  // generate original test result
  // build
  ::system(mBuildCmd.c_str());
  // test
  ::system(mTestCmd.c_str());
  copyTestReportTo(mTestResultDir, expectedDir, mTestResultFileExts);
  // copy test report to expected

  // populate
  auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot,
                                                        mExtensions,
                                                        mExcludes);

  sentinel::SourceLines sourceLines = repo->getSourceLines(mScope);

  auto generator = std::make_shared<sentinel::UniformMutantGenerator>(
      mBuildDir);

  sentinel::MutationFactory mutationFactory(generator);

  auto mutants = mutationFactory.populate(sourceRoot,
                                           sourceLines,
                                           mLimit);
  if (verbose) {
    for (auto& mutant : mutants) {
      std::stringstream buf;
      buf << mutant;
      logger->info(fmt::format("mutant: {}", buf.str()));
    }
  }

  sentinel::Evaluator evaluator(expectedDir, sourceRoot);

  for (auto& m : mutants) {
    // mutate
    repo->getSourceTree()->modify(m, backupDir);
    // build
    int buildExitCode = ::system(mBuildCmd.c_str());
    if (buildExitCode == 0) {
      // test
      ::system(mTestCmd.c_str());
      copyTestReportTo(mTestResultDir, actualDir, mTestResultFileExts);
    }
    // evaluate
    evaluator.compare(m, actualDir);
    // restore mutate
    restoreBackup(backupDir, sourceRoot);
  }

  // TODO(daeseong.seong) : consider rebuilding with original source!!!
  //                        (source file restore was completed already.)

  // report
  sentinel::XMLReport xmlReport(evaluator.getMutationResults(),
    sourceRoot);
  xmlReport.save(outputDir);
  sentinel::HTMLReport htmlReport(evaluator.getMutationResults(),
    sourceRoot);
  htmlReport.save(outputDir);

  htmlReport.printSummary();

  return 0;
}

void CommandStandAlone::copyTestReportTo(const std::string& from,
  const std::string& to, const std::vector<std::string>& exts) {
  auto xmlFiles = sentinel::os::findFilesInDirUsingExt(from, exts);

  fs::remove_all(to);
  fs::create_directories(to);

  for (auto& xmlFile : xmlFiles) {
    // TODO(daeseong.seong): keep relative directory of backupFile
    fs::copy(xmlFile, to);
  }
}

void CommandStandAlone::restoreBackup(const std::string& backup,
  const std::string& srcRoot) {
  auto backupFiles = sentinel::os::findFilesInDir(backup);

  for (auto& backupFile : backupFiles) {
    // TODO(daeseong.seong): keep relative directory of backupFile
    fs::copy(backupFile, srcRoot, fs::copy_options::overwrite_existing);
    fs::remove(backupFile);
  }
}
}  // namespace sentinel
