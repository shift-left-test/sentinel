/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <unistd.h>
#include <sys/wait.h>
#include <term.h>
#include <experimental/filesystem>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/util/signal.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/util/Subprocess.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/CommandRun.hpp"

namespace sentinel {

static const char * cCommandRunLoggerName = "CommandRun";
bool workDirExists = true;
bool backupDirExists = true;
bool expectedDirExists = true;
bool actualDirExists = true;
std::experimental::filesystem::path workDirForSH;
std::experimental::filesystem::path sourceRootForSH;
static bool stopRun = false;

static void signalHandler(int signum) {
  namespace fs = std::experimental::filesystem;

  fs::path backupDir = workDirForSH / "backup";
  if (fs::exists(backupDir)) {
    if (fs::is_directory(backupDir)) {
      CommandRun::restoreBackup(backupDir, sourceRootForSH);
    }
  }
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
    std::cerr << fmt::format("Receive a signal({}).", strsignal(signum)) << std::endl;
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
    "Extentions of source files to be mutated.",
    {'t', "extension"}, {"cxx", "cpp", "cc", "c", "c++", "cu"}),
  mPatterns(parser, "PATTERN",
    "Path or pattern",
    {'p', "pattern"}),
  mExcludes(parser, "PAT",
    "Exclude paths matching fnmatch-style patterns",
    {'e', "exclude"}),
  mLimit(parser, "COUNT",
    "Maximum number of mutants to be generated",
    {'l', "limit"}, 10),
  mBuildCmd(parser, "SH_CMD",
    "Shell command to build source",
    {"build-command"}),
  mTestCmd(parser, "SH_CMD",
    "Shell command to execute test",
    {"test-command"}),
  mTestResultDir(parser, "PATH",
    "Test command output directory",
    {"test-result-dir"}),
  mTestResultFileExts(parser, "EXTENSION",
    "Test command output file extensions.",
    {"test-result-extension"}, {"xml", "XML"}),
  mCoverageFiles(parser, "COV.INFO",
    "lcov-format coverage result file",
    {"coverage"}),
  mGenerator(parser, "gen",
    "Mutant generator type, one of ['uniform', 'random', 'weighted'].",
    {"generator"}, "uniform"),
  mTimeLimitStr(parser, "TIME_SEC",
    R"a1s2(Time limit (sec) for test-command. If 0, there is no time limit. If auto, time limit is automatically set using the test execution time of the original code.)a1s2",
    {"timeout"}, "auto"),
  mKillAfterStr(parser, "TIME_SEC",
    R"asdf(Send SIGKILL if test-command is still running after timeout. If 0, SIGKILL is not sent. This option has no meaning when timeout is set 0.)asdf",
    {"kill-after"}, "60"),
  mSeed(parser, "SEED",
    "Select random seed.",
    {"seed"}, std::random_device {}()) {
}

void CommandRun::setSignalHandler() {
  signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1},
                                    signalHandler);
}

int CommandRun::run() {
  namespace fs = std::experimental::filesystem;
  workDirForSH = fs::current_path() / getWorkDir();
  stopRun = false;
  setSignalHandler();

  try {
    // Directory setting
    std::string sourceRootStr = getSourceRoot();
    std::string workDirStr = getWorkDir();
    std::string outputDirStr = getOutputDir();
    std::string testResultDirStr = getTestResultDir();
    fs::path sourceRoot = fs::canonical(sourceRootStr);
    sourceRootForSH = fs::canonical(sourceRootStr);

    if (!fs::exists(workDirStr)) {
      workDirExists = false;
      fs::create_directories(workDirStr);
    }
    fs::path workDir = fs::canonical(workDirStr);
    std::string backupDir = preProcessWorkDir((workDir / "backup").string(), &backupDirExists, true);
    std::string actualDir = preProcessWorkDir((workDir / "actual").string(), &actualDirExists, false);
    std::string expectedDir = preProcessWorkDir((workDir / "expected").string(), &expectedDirExists, false);

    bool emptyOutputDir = false;
    if (outputDirStr.empty()) {
      outputDirStr = ".";
      emptyOutputDir = true;
    }
    fs::path outputDir = fs::absolute(outputDirStr);

    // Validate build command, test command, test result dir is not null
    std::string buildCmd = getBuildCmd();
    std::string testCmd = getTestCmd();

    if (buildCmd.empty()) {
      throw InvalidArgumentException("Option --build-command is required to be not empty");
    }

    if (testCmd.empty()) {
      throw InvalidArgumentException("Option --test-command is required to be not empty");
    }

    if (testResultDirStr.empty()) {
      throw InvalidArgumentException("Option --test-result-dir is required");
    }

    if (fs::exists(testResultDirStr)) {
      if (!fs::is_directory(testResultDirStr)) {
        throw InvalidArgumentException(fmt::format("The given test result path is not a directory: {0}",
                                                   testResultDirStr));
      }

      if (!fs::is_empty(testResultDirStr)) {
        throw InvalidArgumentException(fmt::format("The given test result path is not empty: {0}",
                                                   testResultDirStr));
      }
    }
    fs::create_directories(testResultDirStr);
    fs::path testResultDir = fs::canonical(testResultDirStr);

    fs::path buildDir = fs::canonical(getBuildDir());
    std::vector<std::string> testResultFileExts = getTestResultFileExts();
    std::string timeLimit = getTestTimeLimit();
    size_t mutantLimit = getMutantLimit();
    std::string killAfter = getKillAfter();
    std::vector<std::string> targetFileExts = getTargetFileExts();
    std::vector<std::string> excludePaths = getExcludePaths();
    std::vector<std::string> diffPatterns = getPatterns();
    std::vector<std::string> coverageFiles = getCoverageFiles();
    std::string scope = getScope();
    std::string generatorStr = getGenerator();
    int randomSeed = getSeed();

    size_t mTimeLimit = 0;
    if (timeLimit != "auto") {
      try {
        mTimeLimit = sentinel::string::stringToInt<size_t>(timeLimit);
      } catch(...) {
        throw InvalidArgumentException(fmt::format(
              R"a1s2(Failed to read timeout option value({}). Please execute "sentinel run --help" and check valid option value.)a1s2",
              timeLimit));
      }
    }

    size_t mKillAfter = 0;
    try {
      mKillAfter = sentinel::string::stringToInt<size_t>(killAfter);
    } catch(...) {
      throw InvalidArgumentException(fmt::format(
            R"a1s2(Failed to read kill-after option value({}). Please execute "sentinel run --help" and check valid option value.)a1s2",
            killAfter));
    }

    // log parsed parameter
    auto logger = Logger::getLogger(cCommandRunLoggerName);
    logger->info(fmt::format("{0:-^{1}}", "", 50));
    logger->info(fmt::format("Source root: \"{}\"", sourceRoot.string()));
    logger->info(fmt::format("Build dir: \"{}\"", buildDir.string()));
    logger->info(fmt::format("Build cmd: \"{}\"", buildCmd));

    logger->info(fmt::format("Test cmd: \"{}\"", testCmd));
    logger->info(fmt::format("Test result dir: \"{}\"", testResultDir.string()));
    logger->info(fmt::format("Test result extension: \"{}\"", sentinel::string::join(", ", testResultFileExts)));
    logger->info(fmt::format("Time limit for test: {}s", std::to_string(mTimeLimit)));
    logger->info(fmt::format("Kill after time limit: {}s", std::to_string(mKillAfter)));
    logger->info(fmt::format("Extentions of source: {}", sentinel::string::join(", ", targetFileExts)));
    logger->info(fmt::format("Exclude patterns: {}", sentinel::string::join(", ", excludePaths)));
    logger->info(fmt::format("Patterns: {}", sentinel::string::join(", ", diffPatterns)));
    logger->info(fmt::format("Coverage files: {}", sentinel::string::join(", ", coverageFiles)));
    logger->info(fmt::format("Diff scope: {}", scope));
    logger->info(fmt::format("Generator: {}", generatorStr));
    logger->info(fmt::format("Max generated mutable: {}", std::to_string(mutantLimit)));
    logger->info(fmt::format("Random seed: {}", std::to_string(randomSeed)));

    logger->info(fmt::format("Work dir: \"{}\"", workDir.string()));
    logger->info(fmt::format("Backup dir: \"{}\"", backupDir));
    logger->info(fmt::format("Expected test result dir: \"{}\"", expectedDir));
    logger->info(fmt::format("Actual test result dir: \"{}\"", actualDir));
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // restore if previous backup is exists
    restoreBackup(backupDir, sourceRoot);

    for (const auto& filename : coverageFiles) {
      if (!fs::exists(filename)) {
        throw InvalidArgumentException(fmt::format("Input coverage file does not exist: {}", filename));
      }
    }

    // generate orignal test result
    if (access(buildDir.c_str(), X_OK) != 0) {
      throw std::runtime_error(fmt::format("fail to change dir {} (cause: {})",
                                           buildDir.c_str(), std::strerror(errno)));
    }
    auto cmdPrefix = fmt::format("cd \"{}\" && ", buildDir.string());

    // build
    logger->info("Building original source code ...");
    logger->info(fmt::format("Source root: {}", sourceRoot.string()));
    logger->info(fmt::format("Build dir: {}", buildDir.string()));
    logger->info(fmt::format("Build cmd: {}", cmdPrefix+buildCmd));
    Subprocess origBuildProc{cmdPrefix + buildCmd};
    origBuildProc.execute();
    if (!origBuildProc.isSuccessfulExit()) {
      throw std::runtime_error("Build FAIL.");
    }
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    // test
    logger->info("Running tests ...");
    logger->info(fmt::format("Build dir: {}", buildDir.string()));
    logger->info(fmt::format("Test cmd: {}", testCmd));
    logger->info(fmt::format("Test result dir: {}", testResultDir.string()));
    logger->info(fmt::format("Test result extension: {}", sentinel::string::join(", ", testResultFileExts)));
    logger->info(fmt::format("Time limit for test: {}s", std::to_string(mTimeLimit)));
    logger->info(fmt::format("Kill after time limit: {}s", std::to_string(mKillAfter)));
    fs::remove_all(testResultDir);
    Subprocess firstTestProc(cmdPrefix + testCmd, mTimeLimit, mKillAfter);

    auto start = std::chrono::steady_clock::now();
    firstTestProc.execute();
    if (timeLimit == "auto") {
      auto end = std::chrono::steady_clock::now();
      auto diff = end - start;
      auto diffSecs = std::chrono::duration<double>(diff).count();
      if (diffSecs < 1.0) {
        mTimeLimit = 1;
      } else {
        mTimeLimit = static_cast<size_t>(ceil(diffSecs * 1.1));
      }
      logger->info(fmt::format("Time limit for test is set to {}(s)", mTimeLimit));
    }

    if (firstTestProc.isTimedOut()) {
      throw std::runtime_error("Timeout occurs when excuting test cmd for original source code.");
    }

    if (!fs::exists(testResultDir)) {
      throw InvalidArgumentException(fmt::format("The test result path does not exist : {0}", testResultDirStr));
    }

    if (!fs::is_directory(testResultDir)) {
      throw InvalidArgumentException(fmt::format("The test result path is not a directory: {0}",
                                                 testResultDir.c_str()));
    }

    if (fs::is_empty(testResultDir)) {
      throw InvalidArgumentException(fmt::format("The test result path is empty: {0}", testResultDir.c_str()));
    }

    // copy test report to expected
    copyTestReportTo(testResultDir, expectedDir, testResultFileExts);

    // populate mutants
    logger->info("Populating mutants ...");
    logger->info(fmt::format("Source root: {}", sourceRoot.string()));
    logger->info(fmt::format("Extentions of source: {}", sentinel::string::join(", ", targetFileExts)));
    logger->info(fmt::format("Exclude patterns: {}", sentinel::string::join(", ", excludePaths)));
    auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot, targetFileExts, diffPatterns, excludePaths);

    logger->info(fmt::format("Diff scope: {}", scope));
    sentinel::SourceLines sourceLines = repo->getSourceLines(scope);

    std::shuffle(std::begin(sourceLines), std::end(sourceLines), std::mt19937(randomSeed));

    std::shared_ptr<MutantGenerator> generator;
    if (generatorStr == "uniform") {
      generator = std::make_shared<sentinel::UniformMutantGenerator>(buildDir);
    } else {
      if (generatorStr == "random") {
        generator = std::make_shared<sentinel::RandomMutantGenerator>(buildDir);
      } else {
        if (generatorStr == "weighted") {
          generator = std::make_shared<sentinel::WeightedMutantGenerator>(buildDir);
        } else {
          throw InvalidArgumentException(fmt::format("Invalid value for generator option: {0}", generatorStr));
        }
      }
    }

    logger->info(fmt::format("Generator: {}", generatorStr));
    sentinel::MutationFactory mutationFactory(generator);
    logger->info(fmt::format("Max generated mutable: {}", mutantLimit));
    auto mutants = mutationFactory.populate(sourceRoot, sourceLines, mutantLimit, randomSeed);

    // Build and execute each selected mutant
    CoverageInfo cov(coverageFiles);
    for (auto& mutant : mutants) {
      logger->info(fmt::format("mutant: {}", mutant.str()));
    }
    logger->info(fmt::format("{0:-^{1}}", "", 50));

    sentinel::Evaluator evaluator(expectedDir, sourceRoot);
    int mutantId = 1;

    for (auto& m : mutants) {
      // Check if mutant is on a covered line
      if (!coverageFiles.empty()) {
        if (!cov.cover(m.getPath().string(), m.getFirst().line)) {
          evaluator.compare(m, actualDir, "uncovered");
          logger->info(fmt::format("{0:-^{1}}", "", 50));
          mutantId++;
          continue;
        }
      }

      // mutate
      std::stringstream buf;
      buf << m;
      logger->info(fmt::format("Creating mutant #{0} : {1}", std::to_string(mutantId), buf.str()));
      repo->getSourceTree()->modify(m, backupDir);
      logger->info(fmt::format("{0:-^{1}}", "", 50));

      // build
      logger->info(fmt::format("Building mutant #{} ...", mutantId));
      logger->info(fmt::format("Build dir: {}", buildDir.string()));
      logger->info(fmt::format("Build cmd: {}", buildCmd));
      Subprocess buildProc(cmdPrefix + buildCmd);
      buildProc.execute();
      std::string testState = "success";
      bool buildSucess = buildProc.isSuccessfulExit();
      if (buildSucess) {
        // test
        logger->info("Building SUCCESS. Testing ...");
        fs::remove_all(testResultDir);

        logger->info(fmt::format("Build dir: {}", buildDir.string()));
        logger->info(fmt::format("Test cmd:{}", testCmd));
        logger->info(fmt::format("Test result dir: {}", testResultDir.string()));
        logger->info(fmt::format("Test result extension: {}", sentinel::string::join(", ", testResultFileExts)));
        logger->info(fmt::format("Time limit for test: {}s", mTimeLimit));
        logger->info(fmt::format("Kill after time limit: {}s", mKillAfter));
        Subprocess proc(cmdPrefix + testCmd, mTimeLimit, mKillAfter);
        proc.execute();
        bool testTimeout = proc.isTimedOut();
        if (testTimeout) {
          testState = "timeout";
          logger->info("Timeout when executing test command.");
          fs::remove_all(actualDir);
          fs::remove_all(testResultDir);
        } else {
          copyTestReportTo(testResultDir, actualDir, testResultFileExts);
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

    // report
    fs::create_directories(outputDir);
    outputDir = fs::canonical(outputDir);

    if (!emptyOutputDir) {
      logger->info(fmt::format("Writing Report to {}", outputDir.string()));
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.save(outputDir);
      sentinel::HTMLReport htmlReport(evaluator.getMutationResults(), sourceRoot);
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
          fs::copy(curPath, to);
        }
      }
    }
  }
}

void CommandRun::restoreBackup(const std::string& backup, const std::string& srcRoot) {
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

std::string CommandRun::preProcessWorkDir(const std::string& target, bool* targetExists, bool isFilledDir) {
  namespace fs =  std::experimental::filesystem;
  if (!fs::exists(target)) {
    *targetExists = false;
    fs::create_directories(target);
  } else {
    if (!fs::is_directory(target)) {
      throw InvalidArgumentException(fmt::format("{0} must be directory.", target));
    }

    if (!isFilledDir && !fs::is_empty(target)) {
      throw InvalidArgumentException(fmt::format("{0} must be empty.", target));
    }
  }
  return fs::canonical(target).string();
}

std::string CommandRun::getSourceRoot() {
  return mSourceRoot.Get();
}

std::string CommandRun::getBuildDir() {
  return mBuildDir.Get();
}

std::string CommandRun::getWorkDir() {
  return mWorkDir.Get();
}

std::string CommandRun::getOutputDir() {
  return mOutputDir.Get();
}

std::string CommandRun::getTestResultDir() {
  return mTestResultDir.Get();
}

std::string CommandRun::getBuildCmd() {
  return mBuildCmd.Get();
}

std::string CommandRun::getTestCmd() {
  return mTestCmd.Get();
}

std::string CommandRun::getGenerator() {
  return mGenerator.Get();
}

std::vector<std::string> CommandRun::getTestResultFileExts() {
  return mTestResultFileExts.Get();
}

std::vector<std::string> CommandRun::getTargetFileExts() {
  return mExtensions.Get();
}

std::vector<std::string> CommandRun::getPatterns() {
  return mPatterns.Get();
}

std::vector<std::string> CommandRun::getExcludePaths() {
  return mExcludes.Get();
}

std::vector<std::string> CommandRun::getCoverageFiles() {
  return mCoverageFiles.Get();
}

std::string CommandRun::getScope() {
  return mScope.Get();
}

size_t CommandRun::getMutantLimit() {
  return mLimit.Get();
}

std::string CommandRun::getTestTimeLimit() {
  return mTimeLimitStr.Get();
}

std::string CommandRun::getKillAfter() {
  return mKillAfterStr.Get();
}

unsigned CommandRun::getSeed() {
  return mSeed.Get();
}

bool CommandRun::getVerbose() {
  return mIsVerbose.Get();
}

}  // namespace sentinel
