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
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/CommandRun.hpp"


namespace sentinel {
static const char * cCommandRunLoggerName = "CommandRun";
static bool workDirExists = true;
static bool backupDirExists = true;
static bool expectedDirExists = true;
static bool actualDirExists = true;
static std::experimental::filesystem::path workDirForSH;

static void signalHandler(int signum) {
  namespace fs = std::experimental::filesystem;
  if (!workDirExists) {
    fs::remove_all(workDirForSH);
  } else {
    if (!backupDirExists) {
      fs::remove_all(workDirForSH / "backup");
    }
    if (!expectedDirExists) {
      fs::remove_all(workDirForSH / "expected");
    }
    if (!actualDirExists) {
      fs::remove_all(workDirForSH / "actual");
    }
  }
  if (signum != SIGUSR1) {
    std::cout << "Receive " << strsignal(signum) << std::endl;
    std::exit(signum);
  }
}

CommandRun::CommandRun(args::Subparser& parser) : Command(parser),
  mBuildDir(parser, "PATH",
    "Directory where compile_commands.json file exists.",
    {'b', "build-dir"}, "."),
  mScope(parser, "SCOPE",
    "Diff scope, one of ['commit', 'all'].",
    {'s', "scope"}, "all"),
  mExtensions(parser, "EXTENSION",
    "Extentions of source files which could be mutated.",
    {'t', "extension"}, {"cxx", "cpp", "cc", "c", "c++", "cu"}),
  mExcludes(parser, "PATH",
    "Exclude file or path",
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
    {"test-result-extention"}, {"xml", "XML"}),
  mGenerator(parser, "gen",
    "Select mutant generator type, one of ['uniform', 'random', 'weighted'].",
    {"generator"}, "uniform") {
}

int CommandRun::run() {
  namespace fs = std::experimental::filesystem;
  workDirForSH = fs::current_path() / mWorkDir.Get();

  std::signal(SIGABRT, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGFPE, signalHandler);
  std::signal(SIGILL, signalHandler);
  std::signal(SIGSEGV, signalHandler);
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGUSR1, signalHandler);

  try {
    if (!fs::exists(mWorkDir.Get())) {
      workDirExists = false;
      fs::create_directories(mWorkDir.Get());
    }
    fs::path workDir = fs::canonical(mWorkDir.Get());
    fs::path sourceRoot = fs::canonical(mSourceRoot.Get());

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
    logger->info(fmt::format("{0:-^{1}}", "", 50));
    logger->info(fmt::format("build dir: {}", mBuildDir.Get()));
    logger->info(fmt::format("build cmd: {}", mBuildCmd.Get()));
    logger->info(fmt::format("test cmd:{}", mTestCmd.Get()));
    logger->info(fmt::format("test result dir: {}", mTestResultDir.Get()));

    if (!fs::exists(mTestResultDir.Get())) {
      throw InvalidArgumentException(fmt::format(
          "The test result path does not exist : {0}",
          mTestResultDir.Get()));
    }

    if (!fs::is_directory(mTestResultDir.Get())) {
      throw InvalidArgumentException(fmt::format(
          "The given test result path is not a directory: {0}",
          mTestResultDir.Get()));
    }

    if (!fs::is_empty(mTestResultDir.Get())) {
      throw InvalidArgumentException(fmt::format(
          "The given test result path is not empty: {0}",
          mTestResultDir.Get()));
    }

    fs::path buildDir = fs::canonical(mBuildDir.Get());
    fs::path testResultDir = fs::canonical(mTestResultDir.Get());

    std::string backupDir = preProcessWorkDir(
        (workDir / "backup").string(), &backupDirExists, true);
    std::string expectedDir = preProcessWorkDir(
        (workDir / "actual").string(), &actualDirExists, false);
    std::string actualDir = preProcessWorkDir(
        (workDir / "expected").string(), &expectedDirExists, false);

    logger->info(fmt::format("backup dir: {}", backupDir));
    logger->info(fmt::format("expected test result dir: {}", expectedDir));
    logger->info(fmt::format("actual test result dir: {}", actualDir));
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // restore if previous backup is exists
    restoreBackup(backupDir, sourceRoot);

    // generate orignal test result
    ::chdir(buildDir.c_str());

    // build
    logger->info("Building original source code ...");
    logger->info(fmt::format("build dir: {}", buildDir.c_str()));
    logger->info(fmt::format("build cmd: {}", mBuildCmd.Get()));
    ::system(mBuildCmd.Get().c_str());
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // test
    logger->info("Running tests ...");
    logger->info(fmt::format("test cmd: {}", mTestCmd.Get()));
    ::system(mTestCmd.Get().c_str());
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // copy test report to expected
    copyTestReportTo(testResultDir, expectedDir, mTestResultFileExts.Get());

    // populate
    logger->info("Populating mutants ...");
    auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot,
                                                          mExtensions.Get(),
                                                          mExcludes.Get());

    sentinel::SourceLines sourceLines = repo->getSourceLines(mScope.Get());
    auto rng = std::default_random_engine{};
    std::shuffle(std::begin(sourceLines), std::end(sourceLines), rng);

    // auto generator = std::make_shared<sentinel::UniformMutantGenerator>(
    //     buildDir);
    std::shared_ptr<MutantGenerator> generator;
    if (mGenerator.Get() == "uniform") {
      generator = std::make_shared<sentinel::UniformMutantGenerator>(
          mBuildDir.Get());
    } else {
      if (mGenerator.Get() == "random") {
        generator = std::make_shared<sentinel::RandomMutantGenerator>(
            mBuildDir.Get());
      } else {
        if (mGenerator.Get() == "weighted") {
          generator = std::make_shared<sentinel::WeightedMutantGenerator>(
              mBuildDir.Get());
        } else {
          throw InvalidArgumentException(fmt::format(
              "Invalid value for generator option: {0}", mGenerator.Get()));
        }
      }
    }

    sentinel::MutationFactory mutationFactory(generator);

    auto mutants = mutationFactory.populate(sourceRoot,
                                             sourceLines,
                                             mLimit.Get());
    if (mIsVerbose.Get()) {
      for (auto& mutant : mutants) {
        logger->info(fmt::format("mutant: {}", mutant.str()));
      }
    }
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    sentinel::Evaluator evaluator(expectedDir, sourceRoot);
    int mutantId = 1;

    for (auto& m : mutants) {
      // mutate
      std::stringstream buf;
      buf << m;
      logger->info(fmt::format("Creating mutant #{0} : {1}",
                               std::to_string(mutantId), buf.str()));
      repo->getSourceTree()->modify(m, backupDir);
      logger->info(fmt::format("{0:-^{1}}", "", 50));

      // build
      logger->info(fmt::format("Building mutant #{} ...", mutantId));
      int buildExitCode = ::system(mBuildCmd.Get().c_str());
      if (buildExitCode == 0) {
        // test
        logger->info("Building SUCCESS. Testing ...");
        fs::remove_all(testResultDir);
        ::system(mTestCmd.Get().c_str());
        copyTestReportTo(testResultDir, actualDir, mTestResultFileExts.Get());
      } else {
        logger->info("Building FAIL.");
      }
      logger->info(fmt::format("{0:-^{1}}", "", 50));

      // evaluate
      logger->info("Evaluating mutant test results ...");
      evaluator.compare(m, actualDir, buildExitCode != 0);
      logger->info(fmt::format("{0:-^{1}}", "", 50));

      // restore mutate
      logger->info("Restoring source code ...");
      restoreBackup(backupDir, sourceRoot);
      logger->info(fmt::format("{0:-^{1}}", "", 50));
      mutantId++;
    }

    // TODO(daeseong.seong) : consider rebuilding with original source!!!
    //                        (source file restore was completed already.)

    // report
    if (!emptyOutputDir) {
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.save(outputDir);
      sentinel::HTMLReport htmlReport(evaluator.getMutationResults(),
          sourceRoot);
      htmlReport.save(outputDir);
      htmlReport.printSummary();
    } else {
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.printSummary();
    }
  } catch(...) {
    std::raise(SIGUSR1);
    throw;
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

  for (const auto& dirent : fs::directory_iterator(backup)) {
    const auto& backupFile = dirent.path();
    if (fs::is_regular_file(backupFile) || fs::is_directory(backupFile)) {
      fs::copy(backupFile, srcRoot / backupFile.filename(),
          fs::copy_options::overwrite_existing | fs::copy_options::recursive);
      fs::remove_all(backupFile);
    }
  }
}

std::string CommandRun::preProcessWorkDir(const std::string& target,
    bool* targetExists, bool isFilledDir) {
  namespace fs =  std::experimental::filesystem;
  if (!fs::exists(target)) {
    *targetExists = false;
    fs::create_directories(target);
  } else {
    if (!fs::is_directory(target)) {
      throw InvalidArgumentException(fmt::format(
            "{0} must be directory.",
            target));
    }
    if (!isFilledDir && !fs::is_empty(target)) {
      throw InvalidArgumentException(fmt::format(
            "{0} must be empty.",
            target));
    }
  }
  return fs::canonical(target).string();
}
}  // namespace sentinel
