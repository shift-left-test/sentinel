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
#include <sys/wait.h>
#include <experimental/filesystem>
#include <exception>
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
#include "sentinel/util/signal.hpp"
#include "sentinel/util/Subprocess.hpp"
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
  std::cout.flush();
  if (signum != SIGUSR1) {
    std::cout <<
      fmt::format("Receive a signal({}).", strsignal(signum)) << std::endl;
    std::exit(EXIT_FAILURE);
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
    {"generator"}, "uniform"),
  mTimeLimit(parser, "TIME_SEC",
    "Time limit (sec) for test-command. If 0, there is no time limit.",
    {"timeout"}, 300),
  mKillAfter(parser, "TIME_SEC",
    R"asdf(Send SIGKILL if test-command is still running after timeout. If 0, SIGKILL is not sent. This option has no meaning when timeout is set 0.)asdf",
    {"kill-after"}, 60) {
}

int CommandRun::run() {
  namespace fs = std::experimental::filesystem;
  workDirForSH = fs::current_path() / mWorkDir.Get();

  signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV,
      SIGTERM, SIGQUIT, SIGHUP, SIGUSR1}, signalHandler);

  try {
    // Directory setting
    fs::path sourceRoot = fs::canonical(mSourceRoot.Get());

    if (!fs::exists(mWorkDir.Get())) {
      workDirExists = false;
      fs::create_directories(mWorkDir.Get());
    }
    fs::path workDir = fs::canonical(mWorkDir.Get());
    std::string backupDir = preProcessWorkDir(
        (workDir / "backup").string(), &backupDirExists, true);
    std::string expectedDir = preProcessWorkDir(
        (workDir / "actual").string(), &actualDirExists, false);
    std::string actualDir = preProcessWorkDir(
        (workDir / "expected").string(), &expectedDirExists, false);

    bool emptyOutputDir = false;
    std::string outputDirStr;
    if (mOutputDir.Get().empty()) {
      outputDirStr = ".";
      emptyOutputDir = true;
    } else {
      outputDirStr = mOutputDir.Get();
    }
    fs::path outputDir = fs::absolute(outputDirStr);

    if (fs::exists(mTestResultDir.Get())) {
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
    }
    fs::create_directories(mTestResultDir.Get());
    fs::path testResultDir = fs::canonical(mTestResultDir.Get());

    fs::path buildDir = fs::canonical(mBuildDir.Get());

    // log parsed parameter
    auto logger = Logger::getLogger(cCommandRunLoggerName);
    logger->info(fmt::format("{0:-^{1}}", "", 50));
    logger->info(fmt::format("Source root: {}", sourceRoot.string()));
    logger->info(fmt::format("Build dir: {}", buildDir.string()));
    logger->info(fmt::format("Build cmd: {}", mBuildCmd.Get()));

    logger->info(fmt::format("Test cmd:{}", mTestCmd.Get()));
    logger->info(fmt::format("Test result dir: {}", testResultDir.string()));
    logger->info(fmt::format("Test result extension: {}",
          sentinel::string::join(", ", mTestResultFileExts.Get())));
    logger->info(fmt::format("Time limit for test: {}s", mTimeLimit.Get()));
    logger->info(fmt::format("Kill after time limit: {}s", mKillAfter.Get()));

    logger->info(fmt::format("Extentions of source: {}",
          sentinel::string::join(", ", mExtensions.Get())));
    logger->info(fmt::format("Exclude path: {}",
          sentinel::string::join(", ", mExcludes.Get())));
    logger->info(fmt::format("Diff scope: {}", mScope.Get()));
    logger->info(fmt::format("Generator: {}", mGenerator.Get()));
    logger->info(fmt::format("Max generated mutable: {}", mLimit.Get()));

    logger->info(fmt::format("Work dir: {}", workDir.string()));
    logger->info(fmt::format("Backup dir: {}", backupDir));
    logger->info(fmt::format("Expected test result dir: {}", expectedDir));
    logger->info(fmt::format("Actual test result dir: {}", actualDir));
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // restore if previous backup is exists
    restoreBackup(backupDir, sourceRoot);

    // generate orignal test result
    if (::chdir(buildDir.c_str()) != 0) {
      throw std::runtime_error(fmt::format("fail to change dir {} (cause: {})",
            buildDir.c_str(), std::strerror(errno)));
    }

    // build
    logger->info("Building original source code ...");
    logger->info(fmt::format("Source root: {}", sourceRoot.string()));
    logger->info(fmt::format("Build dir: {}", buildDir.string()));
    logger->info(fmt::format("Build cmd: {}", mBuildCmd.Get()));
    Subprocess(mBuildCmd.Get()).execute();
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // test
    logger->info("Running tests ...");
    logger->info(fmt::format("Build dir: {}", buildDir.string()));
    logger->info(fmt::format("Test cmd: {}", mTestCmd.Get()));
    logger->info(fmt::format("Test result dir: {}", testResultDir.string()));
    logger->info(fmt::format("Test result extension: {}",
          sentinel::string::join(", ", mTestResultFileExts.Get())));
    logger->info(fmt::format("Time limit for test: {}s", mTimeLimit.Get()));
    logger->info(fmt::format("Kill after time limit: {}s", mKillAfter.Get()));
    fs::remove_all(testResultDir);
    Subprocess firstTestProc(mTestCmd.Get(), mTimeLimit.Get(),
        mKillAfter.Get());
    firstTestProc.execute();
    if (firstTestProc.isTimedOut()) {
      throw std::runtime_error(
          "Timeout occurs when excuting test cmd for original source code.");
    }
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    if (!fs::exists(testResultDir)) {
      throw InvalidArgumentException(fmt::format(
          "The test result path does not exist : {0}",
          mTestResultDir.Get()));
    }

    if (!fs::is_directory(testResultDir)) {
      throw InvalidArgumentException(fmt::format(
            "The test result path is not a directory: {0}",
            testResultDir.c_str()));
    }

    if (fs::is_empty(testResultDir)) {
      throw InvalidArgumentException(fmt::format(
            "The test result path is empty: {0}",
            testResultDir.c_str()));
    }

    // copy test report to expected
    copyTestReportTo(testResultDir, expectedDir, mTestResultFileExts.Get());

    // populate
    logger->info("Populating mutants ...");
    logger->info(fmt::format("Source root: {}", sourceRoot.string()));
    logger->info(fmt::format("Extentions of source: {}",
          sentinel::string::join(", ", mExtensions.Get())));
    logger->info(fmt::format("Exclude path: {}",
          sentinel::string::join(", ", mExcludes.Get())));
    auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot,
                                                          mExtensions.Get(),
                                                          mExcludes.Get());

    logger->info(fmt::format("Diff scope: {}", mScope.Get()));
    sentinel::SourceLines sourceLines = repo->getSourceLines(mScope.Get());
    auto rng = std::default_random_engine{};
    std::shuffle(std::begin(sourceLines), std::end(sourceLines), rng);

    // auto generator = std::make_shared<sentinel::UniformMutantGenerator>(
    //     buildDir);
    std::shared_ptr<MutantGenerator> generator;
    if (mGenerator.Get() == "uniform") {
      generator = std::make_shared<sentinel::UniformMutantGenerator>(buildDir);
    } else {
      if (mGenerator.Get() == "random") {
        generator = std::make_shared<sentinel::RandomMutantGenerator>(
            buildDir);
      } else {
        if (mGenerator.Get() == "weighted") {
          generator = std::make_shared<sentinel::WeightedMutantGenerator>(
              buildDir);
        } else {
          throw InvalidArgumentException(fmt::format(
              "Invalid value for generator option: {0}", mGenerator.Get()));
        }
      }
    }

    logger->info(fmt::format("Generator: {}", mGenerator.Get()));

    sentinel::MutationFactory mutationFactory(generator);

    logger->info(fmt::format("Max generated mutable: {}", mLimit.Get()));
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
      logger->info(fmt::format("Build dir: {}", buildDir.string()));
      logger->info(fmt::format("Build cmd: {}", mBuildCmd.Get()));
      Subprocess buildProc(mBuildCmd.Get());
      buildProc.execute();
      bool buildSucess = buildProc.isSuccessfulExit();
      std::string testState = "success";
      if (buildSucess) {
        // test
        logger->info("Building SUCCESS. Testing ...");
        fs::remove_all(testResultDir);

        logger->info(fmt::format("Build dir: {}", buildDir.string()));
        logger->info(fmt::format("Test cmd:{}", mTestCmd.Get()));
        logger->info(fmt::format("Test result dir: {}",
              testResultDir.string()));
        logger->info(fmt::format("Test result extension: {}",
              sentinel::string::join(", ", mTestResultFileExts.Get())));
        logger->info(fmt::format("Time limit for test: {}s", mTimeLimit.Get()));
        logger->info(fmt::format("Kill after time limit: {}s",
              mKillAfter.Get()));
        Subprocess proc(mTestCmd.Get(), mTimeLimit.Get(), mKillAfter.Get());
        proc.execute();
        bool testTimeout = proc.isTimedOut();
        if (testTimeout) {
          testState = "timeout";
          logger->info("Timeout when executing test command.");
          fs::remove_all(actualDir);
          fs::remove_all(testResultDir);
        } else {
          copyTestReportTo(testResultDir, actualDir, mTestResultFileExts.Get());
          fs::remove_all(testResultDir);
        }
      } else {
        testState = "build_failure";
        logger->info("Building FAIL.");
      }
      logger->info(fmt::format("{0:-^{1}}", "", 50));

      // evaluate
      logger->info("Evaluating mutant test results ...");
      evaluator.compare(m, actualDir, testState);
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

    fs::create_directories(outputDir);
    outputDir = fs::canonical(outputDir);

    if (!emptyOutputDir) {
      logger->info(fmt::format("Writting Report to {}", outputDir.string()));
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.save(outputDir);
      sentinel::HTMLReport htmlReport(evaluator.getMutationResults(),
          sourceRoot);
      htmlReport.save(outputDir);
      logger->info("Print Summary Report");
      htmlReport.printSummary();
    } else {
      logger->info("Print Summary Report");
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.printSummary();
    }
  } catch(...) {
    std::raise(SIGUSR1);
    throw;
  }

  std::raise(SIGUSR1);
  return 0;
}

void CommandRun::copyTestReportTo(const std::string& from,
  const std::string& to, const std::vector<std::string>& exts) {
  namespace fs = std::experimental::filesystem;

  fs::remove_all(to);
  fs::create_directories(to);


  if (fs::exists(from) && fs::is_directory(from)) {
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
