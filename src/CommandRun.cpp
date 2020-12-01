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
#include <unistd.h>
#include <experimental/filesystem>
#include <cstdlib>
#include <map>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/CommandRun.hpp"


namespace sentinel {
const char * cCommandRunLoggerName = "CommandRun";

CommandRun::CommandRun(args::Subparser& parser) : Command(parser),
  mBuildDir(parser, "PATH",
    "Directory where compile_commands.json file exists.",
    {'b', "build-dir"}, "."),
  mScope(parser, "SCOPE",
    "Diff scope, one of ['commit', 'all'].",
    {'s', "scope"}, "all"),
  mExtensions(parser, "EXTENSION",
    "Extentions of source file which could be mutated.",
    {'t', "extension"}, {"cxx", "hxx", "cpp", "hpp", "cc", "hh", "c", "h",
    "c++", "h++", "cu", "cuh"}),
  mExcludes(parser, "PATH",
    "exclude file or path",
    {'e', "exclude"}),
  mLimit(parser, "COUNT",
    "Maximum generated mutable count.",
    {'l', "limit"}, 10),
  mBuildCmd(parser, "SH_CMD",
    "Shell command to build source",
    {"build-command"}, args::Options::Required),
  mTestCmd(parser, "SH_CMD",
    "Shell command to execute test",
    {"test-command"}, args::Options::Required),
  mTestResultDir(parser, "PATH",
    "Test command output directory",
    {"test-result-dir"}, args::Options::Required),
  mTestResultFileExts(parser, "EXTENSION",
    "Test command output file extensions.",
    {"test-result-extention"}, {"xml", "XML"}) {
}

int CommandRun::run() {
  namespace fs =  std::experimental::filesystem;
  fs::path sourceRoot = fs::canonical(mSourceRoot.Get());
  fs::path workDir = fs::canonical(mWorkDir.Get());
  bool emptyOutputDir = false;
  std::string outputDirStr;
  if (mOutputDir.Get().empty()) {
    outputDirStr = ".";
    emptyOutputDir = true;
  } else {
    outputDirStr = mOutputDir.Get();
  }
  fs::create_directories(outputDirStr);
  fs::path outputDir = fs::canonical(outputDirStr);

  auto logger = Logger::getLogger(cCommandRunLoggerName);
  logger->info(fmt::format("build dir: {}", mBuildDir.Get()));
  logger->info(fmt::format("build cmd: {}", mBuildCmd.Get()));
  logger->info(fmt::format("test cmd:{}", mTestCmd.Get()));
  logger->info(fmt::format("test result dir: {}", mTestResultDir.Get()));

  fs::path buildDir = fs::canonical(mBuildDir.Get());
  fs::create_directories(mTestResultDir.Get());
  fs::path testResultDir = fs::canonical(mTestResultDir.Get());
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

  ::chdir(buildDir.c_str());
  // generate original test result
  // build
  ::system(mBuildCmd.Get().c_str());
  // test
  ::system(mTestCmd.Get().c_str());
  copyTestReportTo(testResultDir, expectedDir, mTestResultFileExts.Get());
  // copy test report to expected

  // populate
  auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot,
                                                        mExtensions.Get(),
                                                        mExcludes.Get());

  sentinel::SourceLines sourceLines = repo->getSourceLines(mScope.Get());

  auto generator = std::make_shared<sentinel::UniformMutantGenerator>(
      buildDir);

  sentinel::MutationFactory mutationFactory(generator);

  auto mutants = mutationFactory.populate(sourceRoot,
                                           sourceLines,
                                           mLimit.Get());
  if (mIsVerbose.Get()) {
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
    int buildExitCode = ::system(mBuildCmd.Get().c_str());
    if (buildExitCode == 0) {
      // test
      fs::remove_all(testResultDir);
      ::system(mTestCmd.Get().c_str());
      copyTestReportTo(testResultDir, actualDir, mTestResultFileExts.Get());
    }
    // evaluate
    evaluator.compare(m, actualDir, buildExitCode != 0);
    // restore mutate
    restoreBackup(backupDir, sourceRoot);
  }

  // TODO(daeseong.seong) : consider rebuilding with original source!!!
  //                        (source file restore was completed already.)

  // report
  if (!emptyOutputDir) {
    sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
    xmlReport.save(outputDir);
    sentinel::HTMLReport htmlReport(evaluator.getMutationResults(), sourceRoot);
    htmlReport.save(outputDir);
    htmlReport.printSummary();
  } else {
    sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
    xmlReport.printSummary();
  }

  return 0;
}

void CommandRun::copyTestReportTo(const std::string& from,
  const std::string& to, const std::vector<std::string>& exts) {
  namespace fs = std::experimental::filesystem;

  fs::remove_all(to);
  fs::create_directories(to);

  for (const auto& dirent : fs::recursive_directory_iterator(from)) {
    const auto& curPath = dirent.path();
    std::string curExt = curPath.extension().string();
    std::transform(curExt.begin(), curExt.end(), curExt.begin(),
        [](unsigned char c) { return std::tolower(c); });
    if (fs::is_regular_file(curPath)) {
      bool copyFlag = false;
      if (exts.empty()) {
        copyFlag = true;
      } else {
        for (const auto& t : exts) {
          std::string tmp("." + t);
          std::transform(tmp.begin(), tmp.end(), tmp.begin(),
              [](unsigned char c) { return std::tolower(c); });
          if (tmp == curExt) {
            copyFlag = true;
            break;
          }
        }
      }

      if (copyFlag) {
        // TODO(daeseong.seong): keep relative directory of backupFile
        fs::copy(curPath, to);
      }
    }
  }
}

void CommandRun::restoreBackup(const std::string& backup,
  const std::string& srcRoot) {
  namespace fs = std::experimental::filesystem;

  for (const auto& dirent : fs::recursive_directory_iterator(backup)) {
    const auto& backupFile = dirent.path();
    if (fs::is_regular_file(backupFile)) {
      // TODO(daeseong.seong): keep relative directory of backupFile
      fs::copy(backupFile, srcRoot, fs::copy_options::overwrite_existing);
      fs::remove(backupFile);
    }
  }
}
}  // namespace sentinel
