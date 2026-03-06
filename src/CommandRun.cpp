/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <unistd.h>
#include <sys/wait.h>
#include <experimental/filesystem>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SentinelConfig.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/util/signal.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/util/Subprocess.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/CommandRun.hpp"

namespace sentinel {

static const char* cCommandRunLoggerName = "CommandRun";
static std::experimental::filesystem::path backupDirForSH;
static std::experimental::filesystem::path sourceRootForSH;
static StatusLine* gStatusLineForSH = nullptr;

static void signalHandler(int signum) {
  namespace fs = std::experimental::filesystem;

  if (!backupDirForSH.empty() && fs::exists(backupDirForSH) && fs::is_directory(backupDirForSH)) {
    CommandRun::restoreBackup(backupDirForSH.string(), sourceRootForSH.string());
  }
  if (gStatusLineForSH != nullptr) {
    gStatusLineForSH->disable();
    gStatusLineForSH = nullptr;
  }
  std::cout.flush();
  if (signum != SIGUSR1) {
    std::cerr << fmt::format("Receive a signal({}).", strsignal(signum)) << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

static const char* const kYamlTemplate =
    "# sentinel.yaml — full configuration template\n"
    "#\n"
    "# Uncomment and edit the options you need.\n"
    "# CLI arguments always take priority over values in this file.\n"
    "\n"
    "# --- Shared options (all commands) ---\n"
    "\n"
    "# Source root directory (equivalent to --source-dir CLI option)\n"
    "# source-root: ./\n"
    "\n"
    "# Enable verbose logging\n"
    "# verbose: false\n"
    "\n"
    "# Suppress build/test log output to the terminal (status line still shows progress)\n"
    "# silent: false\n"
    "\n"
    "# Enable debug logging\n"
    "# debug: false\n"
    "\n"
    "# Workspace directory for all sentinel run artifacts\n"
    "# workspace: ./sentinel_workspace\n"
    "\n"
    "# Directory for output reports\n"
    "# output-dir: ./sentinel_output\n"
    "\n"
    "# Change to this directory before running\n"
    "# cwd: .\n"
    "\n"
    "# --- sentinel run options ---\n"
    "\n"
    "# Path to the test binary directory\n"
    "# binary-dir: .\n"
    "\n"
    "# Explicit path to directory containing compile_commands.json\n"
    "# compiledb: .\n"
    "\n"
    "# Diff scope: 'commit' (changed lines only) or 'all' (entire codebase)\n"
    "# scope: all\n"
    "\n"
    "# Source file extensions to mutate\n"
    "# extension:\n"
    "#   - cpp\n"
    "#   - cxx\n"
    "#   - cc\n"
    "#   - c\n"
    "#   - c++\n"
    "#   - cu\n"
    "\n"
    "# Paths or glob patterns to constrain the diff\n"
    "# pattern: []\n"
    "\n"
    "# Paths excluded from mutation (fnmatch-style patterns)\n"
    "# exclude: []\n"
    "\n"
    "# Maximum number of mutants to generate\n"
    "# limit: 10\n"
    "\n"
    "# Shell command to build the source\n"
    "# build-command: make\n"
    "\n"
    "# Shell command to execute tests\n"
    "# test-command: make test\n"
    "\n"
    "# Path to the test report directory\n"
    "# test-report-dir: ./test-results\n"
    "\n"
    "# File extension of the test report\n"
    "# test-report-extension:\n"
    "#   - xml\n"
    "\n"
    "# lcov-format coverage result files\n"
    "# coverage: []\n"
    "\n"
    "# Mutant generator type: 'uniform', 'random', or 'weighted'\n"
    "# generator: uniform\n"
    "\n"
    "# Time limit (seconds) for the test command.\n"
    "# 0 = no limit, 'auto' = automatically determined from baseline run time.\n"
    "# timeout: auto\n"
    "\n"
    "# Seconds to wait after timeout before sending SIGKILL. 0 = disabled.\n"
    "# kill-after: 60\n"
    "\n"
    "# Random seed for mutant selection ('auto' = pick randomly)\n"
    "# seed: auto\n"
    "\n"
    "# Disable the terminal status line even when stdout is a TTY\n"
    "# no-statusline: false\n"
    "\n"
    "# Mutation operators to use. Omit to use all operators.\n"
    "# Valid values: AOR, BOR, LCR, ROR, SDL, SOR, UOI\n"
    "# operator:\n"
    "#   - AOR\n"
    "#   - BOR\n"
    "#   - LCR\n"
    "#   - ROR\n"
    "#   - SDL\n"
    "#   - SOR\n"
    "#   - UOI\n";

CommandRun::CommandRun(args::Group& parser) :
    Command(parser),
    mGroupRunCtrl(parser, "Run options:"),
    mGroupBuildTest(parser, "Build & test options:"),
    mGroupMutation(parser, "Mutation options:"),
    mConfigFile(mGroupRunCtrl, "PATH", "Path to a YAML config file (default: sentinel.yaml in current directory)",
                {"config"}),
    mInit(mGroupRunCtrl, "init", "Write a sentinel.yaml config template to the current directory and exit",
          {"init"}),
    mNoStatusLine(mGroupRunCtrl, "no-statusline", "Disable the live status line shown in TTY mode",
                  {"no-statusline"}),
    mBuildCmd(mGroupBuildTest, "CMD", "Shell command to build the project", {"build-command"}),
    mBuildDir(mGroupBuildTest, "PATH", "Path to the test binary directory",
              {'b', "binary-dir"}, "."),
    mCompileDbDir(mGroupBuildTest, "PATH", "Path to the directory containing compile_commands.json.",
                  {"compiledb"}),
    mTestCmd(mGroupBuildTest, "CMD", "Shell command to run tests", {"test-command"}),
    mTestResultDir(mGroupBuildTest, "PATH", "Path to the test report directory",
                   {"test-report-dir"}),
    mTestResultFileExts(mGroupBuildTest, "EXT", "File extension of the test report",
                        {"test-report-extension"}, {"xml", "XML"}),
    mTimeLimitStr(mGroupBuildTest, "SEC",
                  "Test time limit in seconds; 0 = no limit, auto = 2x baseline run time",
                  {"timeout"}, "auto"),
    mKillAfterStr(mGroupBuildTest, "SEC",
                  "Seconds to wait after timeout before sending SIGKILL (0 = disabled)",
                  {"kill-after"}, "60"),
    mScope(mGroupMutation, "SCOPE", "Mutation scope: 'commit' (changed lines only) or 'all' (entire codebase)",
           {'s', "scope"}, "all"),
    mExtensions(mGroupMutation, "EXT", "Source file extensions to mutate", {'t', "extension"},
                {"cxx", "cpp", "cc", "c", "c++", "cu"}),
    mPatterns(mGroupMutation, "EXPR", "Paths or glob patterns to constrain the mutation scope",
              {'p', "pattern"}),
    mExcludes(mGroupMutation, "EXPR", "Exclude files/directories matching fnmatch-style patterns",
              {'e', "exclude"}),
    mLimit(mGroupMutation, "N", "Maximum number of mutants to generate", {'l', "limit"}, 10),
    mGenerator(mGroupMutation, "TYPE", "Mutant selection strategy: 'uniform', 'random', or 'weighted'",
               {"generator"}, "uniform"),
    mSeed(mGroupMutation, "N", "Random seed for mutant selection ('auto' = pick randomly)", {"seed"}, "auto"),
    mOperators(mGroupMutation, "OP",
               "Mutation operators to apply (AOR BOR LCR ROR SDL SOR UOI); defaults to all",
               {"operator"}),
    mCoverageFiles(mGroupMutation, "FILE",
                   "lcov coverage info file; limits mutation to covered lines only", {"coverage"}) {}

void CommandRun::init() {
  namespace fs = std::experimental::filesystem;

  if (!mInit) {
    // Determine config file path (resolved in the original CWD, before --cwd takes effect).
    std::string configPath;
    if (mConfigFile) {
      configPath = mConfigFile.Get();
      if (!fs::exists(configPath)) {
        throw std::runtime_error(fmt::format("Config file not found: {}", configPath));
      }
    } else if (fs::exists("sentinel.yaml")) {
      configPath = "sentinel.yaml";
    }

    // Load YAML config if a file was found or explicitly specified.
    if (!configPath.empty()) {
      mConfig = SentinelConfig::loadFromFile(configPath);
    }

    // Apply YAML cwd if CLI did not specify --cwd.
    if (!mCwd && mConfig && mConfig->cwd && !mConfig->cwd->empty()) {
      std::error_code ec;
      fs::current_path(*mConfig->cwd, ec);
      if (ec) {
        throw std::runtime_error(fmt::format("Failed to change directory to '{}': {}", *mConfig->cwd, ec.message()));
      }
    }
  }

  // Call base init: applies CLI --cwd and sets log level from CLI -v/--debug.
  Command::init();

  // Apply YAML log level only if CLI did not set verbose or debug.
  if (!mInit && !mIsDebug && !mIsVerbose && mConfig) {
    if (mConfig->debug.value_or(false)) {
      sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::DEBUG);
    } else if (mConfig->verbose.value_or(false)) {
      sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::VERBOSE);
    }
  }
}

void CommandRun::setSignalHandler() {
  signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1},
                                    signalHandler);
}

// Build YAML content for workspace/sentinel.yaml from all resolved run options.
// The timeout value must already be computed (no "auto").
static std::string buildWorkspaceYaml(const std::string& sourceRoot, const std::string& buildDir,
                                      const std::string& compileDbDir, const std::string& scope,
                                      const std::vector<std::string>& extensions,
                                      const std::vector<std::string>& patterns,
                                      const std::vector<std::string>& excludes, size_t limit,
                                      const std::string& buildCmd, const std::string& testCmd,
                                      const std::string& testResultDir,
                                      const std::vector<std::string>& testResultFileExts,
                                      const std::vector<std::string>& coverageFiles,
                                      const std::string& generator, size_t timeout, size_t killAfter,
                                      unsigned seed, const std::vector<std::string>& operators,
                                      const std::string& outputDir, bool verbose, bool debug,
                                      bool noStatusLine, bool silent) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "source-root" << YAML::Value << sourceRoot;
  out << YAML::Key << "verbose" << YAML::Value << verbose;
  out << YAML::Key << "debug" << YAML::Value << debug;
  out << YAML::Key << "no-statusline" << YAML::Value << noStatusLine;
  out << YAML::Key << "silent" << YAML::Value << silent;
  if (!outputDir.empty()) {
    out << YAML::Key << "output-dir" << YAML::Value << outputDir;
  }
  out << YAML::Key << "binary-dir" << YAML::Value << buildDir;
  out << YAML::Key << "compiledb" << YAML::Value << compileDbDir;
  out << YAML::Key << "scope" << YAML::Value << scope;
  out << YAML::Key << "extension" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : extensions) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "pattern" << YAML::Value << YAML::BeginSeq;
  for (const auto& p : patterns) out << p;
  out << YAML::EndSeq;
  out << YAML::Key << "exclude" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : excludes) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "limit" << YAML::Value << limit;
  out << YAML::Key << "build-command" << YAML::Value << buildCmd;
  out << YAML::Key << "test-command" << YAML::Value << testCmd;
  out << YAML::Key << "test-report-dir" << YAML::Value << testResultDir;
  out << YAML::Key << "test-report-extension" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : testResultFileExts) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "coverage" << YAML::Value << YAML::BeginSeq;
  for (const auto& c : coverageFiles) out << c;
  out << YAML::EndSeq;
  out << YAML::Key << "generator" << YAML::Value << generator;
  out << YAML::Key << "timeout" << YAML::Value << timeout;
  out << YAML::Key << "kill-after" << YAML::Value << killAfter;
  out << YAML::Key << "seed" << YAML::Value << seed;
  out << YAML::Key << "operator" << YAML::Value << YAML::BeginSeq;
  for (const auto& op : operators) out << op;
  out << YAML::EndSeq;
  out << YAML::EndMap;
  return out.c_str();
}

int CommandRun::run() {
  if (mInit) {
    static const char* const kConfigFileName = "sentinel.yaml";
    namespace fs = std::experimental::filesystem;
    if (fs::exists(kConfigFileName)) {
      fmt::print("'{}' already exists. Overwrite? [y/N] ", kConfigFileName);
      std::string answer;
      std::getline(std::cin, answer);
      if (answer != "y" && answer != "Y") {
        fmt::print("Aborted.\n");
        return 0;
      }
    }
    std::ofstream out(kConfigFileName);
    if (!out) {
      throw std::runtime_error(fmt::format("Failed to create '{}'", kConfigFileName));
    }
    out << kYamlTemplate;
    fmt::print("Generated '{}'\n", kConfigFileName);
    return 0;
  }

  namespace fs = std::experimental::filesystem;

  StatusLine statusLine;
  gStatusLineForSH = &statusLine;

  // ── STEP 1: Workspace setup ─────────────────────────────────────────────────

  fs::path workDirPath = fs::absolute(getWorkDir());
  Workspace ws(workDirPath);

  bool resuming = false;
  SentinelConfig activeConfig;

  if (ws.hasPreviousRun()) {
    fmt::print("Previous run found in '{}'. Resume? [Y/n] ", workDirPath.string());
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty() || answer == "Y" || answer == "y") {
      resuming = true;
      activeConfig = SentinelConfig::loadFromFile((ws.getRoot() / "sentinel.yaml").string());
      fmt::print("Resuming previous run.\n");
    } else {
      ws.initialize();
      fmt::print("Starting fresh run.\n");
    }
  } else {
    ws.initialize();
  }

  // ── STEP 2: Option resolution ───────────────────────────────────────────────
  // In resume mode all options come from workspace/sentinel.yaml exclusively.
  // In fresh mode they come from CLI > project sentinel.yaml > defaults.

  std::string sourceRootStr;
  std::string buildDirStr;
  std::string compileDbStr;
  std::string buildCmd;
  std::string testCmd;
  std::string testResultDirStr;
  std::vector<std::string> testResultFileExts;
  std::vector<std::string> coverageFiles;
  std::string scope;
  std::string generatorStr;
  size_t mutantLimit;
  size_t mTimeLimit = 0;
  size_t mKillAfter;
  unsigned randomSeed;
  std::vector<std::string> targetFileExts;
  std::vector<std::string> excludePaths;
  std::vector<std::string> diffPatterns;
  std::vector<std::string> operators;
  std::string outputDirStr;

  if (resuming) {
    sourceRootStr = activeConfig.sourceRoot.value_or(".");
    buildDirStr = activeConfig.buildDir.value_or(".");
    compileDbStr = activeConfig.compileDbDir.value_or("");
    buildCmd = activeConfig.buildCmd.value_or("");
    testCmd = activeConfig.testCmd.value_or("");
    testResultDirStr = activeConfig.testResultDir.value_or("");
    testResultFileExts = activeConfig.testResultFileExts.value_or(std::vector<std::string>{"xml", "XML"});
    coverageFiles = activeConfig.coverageFiles.value_or(std::vector<std::string>{});
    scope = activeConfig.scope.value_or("all");
    generatorStr = activeConfig.generator.value_or("uniform");
    mutantLimit = activeConfig.limit.value_or(10);
    mTimeLimit = sentinel::string::stringToInt<size_t>(activeConfig.timeLimit.value_or("0"));
    mKillAfter = sentinel::string::stringToInt<size_t>(activeConfig.killAfter.value_or("60"));
    randomSeed = activeConfig.seed.value_or(0);
    targetFileExts = activeConfig.extensions.value_or(std::vector<std::string>{"cxx", "cpp", "cc", "c", "c++", "cu"});
    excludePaths = activeConfig.excludes.value_or(std::vector<std::string>{});
    diffPatterns = activeConfig.patterns.value_or(std::vector<std::string>{});
    operators = activeConfig.operators.value_or(std::vector<std::string>{});
    outputDirStr = activeConfig.outputDir.value_or("");
  } else {
    sourceRootStr = getSourceRoot();
    buildDirStr = getBuildDir();
    compileDbStr = getCompileDbDir();
    buildCmd = getBuildCmd();
    testCmd = getTestCmd();
    testResultDirStr = getTestResultDir();
    testResultFileExts = getTestResultFileExts();
    coverageFiles = getCoverageFiles();
    scope = getScope();
    generatorStr = getGenerator();
    mutantLimit = getMutantLimit();
    mKillAfter = sentinel::string::stringToInt<size_t>(getKillAfter());
    randomSeed = getSeed();
    targetFileExts = getTargetFileExts();
    excludePaths = getExcludePaths();
    diffPatterns = getPatterns();
    operators = getOperators();
    outputDirStr = getOutputDir();
  }

  // Determine silent mode (suppress build/test subprocess output to terminal)
  bool silent = mSilent.Get() || (mConfig && mConfig->silent.value_or(false));
  if (resuming) {
    silent = activeConfig.silent.value_or(false);
  }

  // Validate required options
  if (buildCmd.empty()) {
    throw InvalidArgumentException("Option --build-command is required to be not empty");
  }
  if (testCmd.empty()) {
    throw InvalidArgumentException("Option --test-command is required to be not empty");
  }
  if (testResultDirStr.empty()) {
    throw InvalidArgumentException("Option --test-report-dir is required");
  }

  // Only parse timeout string for fresh runs (resume uses computed integer directly)
  std::string timeLimitStr;
  if (!resuming) {
    timeLimitStr = getTestTimeLimit();
    if (timeLimitStr != "auto") {
      try {
        mTimeLimit = sentinel::string::stringToInt<size_t>(timeLimitStr);
      } catch (...) {
        throw InvalidArgumentException(fmt::format(
            R"a1s2(Failed to read timeout option value({}). Please execute "sentinel run --help" and check valid option value.)a1s2",
            timeLimitStr));
      }
    }
  }

  // Canonicalize paths
  fs::path sourceRoot = fs::canonical(sourceRootStr);
  sourceRootForSH = sourceRoot;
  backupDirForSH = ws.getBackupDir();
  setSignalHandler();

  fs::path buildDir = fs::canonical(buildDirStr);
  if (compileDbStr.empty()) {
    compileDbStr = buildDirStr;
  }

  // testResultDir: validation in fresh mode; ensure exists for both modes
  if (!resuming) {
    if (fs::exists(testResultDirStr)) {
      if (!fs::is_directory(testResultDirStr)) {
        throw InvalidArgumentException(
            fmt::format("The given test result path is not a directory: {0}", testResultDirStr));
      }
      if (!fs::is_empty(testResultDirStr)) {
        fmt::print("The given test result path is not empty: {0}\nDelete and continue? [y/N] ", testResultDirStr);
        std::string answer;
        std::getline(std::cin, answer);
        if (answer != "y" && answer != "Y") {
          throw InvalidArgumentException(
              fmt::format("The given test result path is not empty: {0}", testResultDirStr));
        }
        fs::remove_all(testResultDirStr);
      }
    }
  }
  fs::create_directories(testResultDirStr);
  fs::path testResultDir = fs::canonical(testResultDirStr);

  bool emptyOutputDir = outputDirStr.empty();
  fs::path outputDir = emptyOutputDir ? fs::absolute(".") : fs::absolute(outputDirStr);

  auto logger = Logger::getLogger(cCommandRunLoggerName);
  logger->verbose(fmt::format("source root:        {}", sourceRoot.string()));
  logger->verbose(fmt::format("build dir:          {}", buildDir.string()));
  logger->verbose(fmt::format("compiledb dir:      {}", compileDbStr));
  logger->verbose(fmt::format("build cmd:          {}", buildCmd));
  logger->verbose(fmt::format("test cmd:           {}", testCmd));
  logger->verbose(fmt::format("test result dir:    {}", testResultDir.string()));
  logger->verbose(fmt::format("test result exts:   {}", sentinel::string::join(", ", testResultFileExts)));
  logger->verbose(fmt::format("timeout:            {}s", mTimeLimit));
  logger->verbose(fmt::format("kill-after:         {}s", mKillAfter));
  logger->verbose(fmt::format("source extensions:  {}", sentinel::string::join(", ", targetFileExts)));
  logger->verbose(fmt::format("exclude patterns:   {}", sentinel::string::join(", ", excludePaths)));
  logger->verbose(fmt::format("patterns:           {}", sentinel::string::join(", ", diffPatterns)));
  logger->verbose(fmt::format("coverage files:     {}", sentinel::string::join(", ", coverageFiles)));
  logger->verbose(fmt::format("scope:              {}", scope));
  logger->verbose(fmt::format("generator:          {}", generatorStr));
  logger->verbose(fmt::format("mutant limit:       {}", mutantLimit));
  logger->verbose(fmt::format("random seed:        {}", randomSeed));
  logger->verbose(fmt::format("workspace:          {}", ws.getRoot().string()));
  logger->verbose(fmt::format("resuming:           {}", resuming ? "yes" : "no"));

  bool disableStatusLine = static_cast<bool>(mNoStatusLine) || (mConfig && mConfig->noStatusLine.value_or(false));
  if (resuming) {
    disableStatusLine = activeConfig.noStatusLine.value_or(false);
  }
  if (!disableStatusLine) {
    statusLine.enable();
  }
  statusLine.setPhase(StatusLine::Phase::BUILD_ORIG);

  auto cmdPrefix = fmt::format("cd \"{}\" && ", buildDir.string());

  for (const auto& filename : coverageFiles) {
    if (!fs::exists(filename)) {
      throw InvalidArgumentException(fmt::format("Input coverage file does not exist: {}", filename));
    }
  }

  try {
    // ── STEP 3: Original build & test ──────────────────────────────────────────
    // Skip if resuming and original results already exist.
    bool origDone = resuming && fs::exists(ws.getOriginalDir() / "build.log") &&
                    fs::exists(ws.getOriginalResultsDir()) && !fs::is_empty(ws.getOriginalResultsDir());

    if (!origDone) {
      if (access(buildDir.c_str(), X_OK) != 0) {
        throw std::runtime_error(
            fmt::format("fail to change dir {} (cause: {})", buildDir.c_str(), std::strerror(errno)));
      }

      // build
      logger->info("Building original source code...");
      Subprocess origBuildProc(cmdPrefix + buildCmd, 0, 0, (ws.getOriginalDir() / "build.log").string(), silent);
      origBuildProc.execute();
      if (!origBuildProc.isSuccessfulExit()) {
        throw std::runtime_error("Build failed.");
      }

      // test
      logger->info("Running original tests...");
      statusLine.setPhase(StatusLine::Phase::TEST_ORIG);
      fs::remove_all(testResultDir);
      Subprocess firstTestProc(cmdPrefix + testCmd, mTimeLimit, mKillAfter,
                               (ws.getOriginalDir() / "test.log").string(), silent);

      auto start = std::chrono::steady_clock::now();
      firstTestProc.execute();

      if (!resuming && timeLimitStr == "auto") {
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        auto diffSecs = std::chrono::duration<double>(diff).count();
        if (diffSecs < 1.0) {
          mTimeLimit = 1;
        } else {
          mTimeLimit = static_cast<size_t>(ceil(diffSecs * 2.0));
        }
        logger->info(fmt::format("Auto timeout: {}s (baseline {:.1f}s x2)", mTimeLimit, diffSecs));
      }

      if (firstTestProc.isTimedOut()) {
        throw std::runtime_error("Timeout occurs when excuting test cmd for original source code.");
      }

      if (!fs::exists(testResultDir)) {
        throw InvalidArgumentException(fmt::format("The test result path does not exist : {0}", testResultDirStr));
      }
      if (!fs::is_directory(testResultDir)) {
        throw InvalidArgumentException(
            fmt::format("The test result path is not a directory: {0}", testResultDir.c_str()));
      }
      if (fs::is_empty(testResultDir)) {
        throw InvalidArgumentException(fmt::format("The test result path is empty: {0}", testResultDir.c_str()));
      }

      // Copy baseline test results to workspace/original/results/
      copyTestReportTo(testResultDir.string(), ws.getOriginalResultsDir().string(), testResultFileExts);

      // Save workspace config now that the baseline phase is complete.
      // From this point on, hasPreviousRun() will return true and resume is possible.
      ws.saveConfig(buildWorkspaceYaml(sourceRoot.string(), buildDirStr, compileDbStr, scope, targetFileExts,
                                       diffPatterns, excludePaths, mutantLimit, buildCmd, testCmd, testResultDirStr,
                                       testResultFileExts, coverageFiles, generatorStr, mTimeLimit, mKillAfter,
                                       randomSeed, operators, outputDirStr, mIsVerbose.Get(), mIsDebug.Get(),
                                       disableStatusLine, silent));
    }

    // Restore any leftover backup from a previously interrupted run
    restoreBackup(ws.getBackupDir().string(), sourceRoot.string());

    // ── STEP 4: Populate or reload mutants ─────────────────────────────────────

    std::vector<std::pair<int, Mutant>> indexedMutants;

    if (resuming) {
      indexedMutants = ws.loadMutants();
      logger->info(fmt::format("Loaded {} mutants from workspace.", indexedMutants.size()));
      statusLine.setTotalMutants(indexedMutants.size());
    } else {
      statusLine.setPhase(StatusLine::Phase::POPULATE);
      logger->info("Generating mutants...");
      auto repo =
          std::make_unique<sentinel::GitRepository>(sourceRoot, targetFileExts, diffPatterns, excludePaths);
      sentinel::SourceLines sourceLines = repo->getSourceLines(scope);
      std::shuffle(std::begin(sourceLines), std::end(sourceLines), std::mt19937(randomSeed));

      auto generator = MutantGenerator::getInstance(generatorStr, compileDbStr);

      static const std::vector<std::string> kValidOperators = {"AOR", "BOR", "LCR", "ROR", "SDL", "SOR", "UOI"};
      for (const auto& op : operators) {
        if (std::find(kValidOperators.begin(), kValidOperators.end(), op) == kValidOperators.end()) {
          throw InvalidArgumentException(
              fmt::format("Invalid operator: '{}'. Valid operators: AOR, BOR, LCR, ROR, SDL, SOR, UOI", op));
        }
      }
      generator->setOperators(operators);

      sentinel::MutationFactory mutationFactory(generator);
      auto mutants = mutationFactory.populate(sourceRoot, sourceLines, mutantLimit, randomSeed);

      int id = 1;
      for (auto& m : mutants) {
        logger->verbose(fmt::format("mutant: {}", m.str()));
        ws.createMutant(id, m);
        indexedMutants.emplace_back(id, m);
        id++;
      }
      statusLine.setTotalMutants(indexedMutants.size());
    }

    // ── STEP 5: Mutant loop ─────────────────────────────────────────────────────

    fs::path actualDir = ws.getRoot() / "actual";
    auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot, targetFileExts, diffPatterns, excludePaths);
    sentinel::Evaluator evaluator(ws.getOriginalResultsDir().string(), sourceRoot.string());
    CoverageInfo cov(coverageFiles);

    statusLine.setPhase(StatusLine::Phase::MUTANT);

    size_t totalMutants = indexedMutants.size();
    logger->info(fmt::format("Evaluating {} mutant{}...", totalMutants, totalMutants == 1 ? "" : "s"));

    for (auto& [id, m] : indexedMutants) {
      // Already completed in a previous run: inject stored result and skip
      if (ws.isDone(id)) {
        MutationResult prevResult = ws.getDoneResult(id);
        evaluator.injectResult(prevResult);
        logger->verbose(fmt::format("Mutant #{} already done: {}. Skipping.", id,
                                    MutationStateToStr(prevResult.getMutationState())));
        statusLine.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                                 m.getFirst().line);
        statusLine.recordResult(static_cast<int>(prevResult.getMutationState()));
        continue;
      }

      // Interrupted mid-mutation in a previous run: restore source before re-running
      if (ws.isLocked(id)) {
        logger->info(fmt::format("Mutant #{} was interrupted mid-run. Restoring source...", id));
        restoreBackup(ws.getBackupDir().string(), sourceRoot.string());
        ws.clearLock(id);
      }

      // Progress indicator (always visible at INFO level)
      logger->info(fmt::format("[{}/{}] {} @ {}:{}",
                               id, totalMutants,
                               m.getOperator(),
                               m.getPath().filename().string(),
                               m.getFirst().line));

      // Coverage check
      if (!coverageFiles.empty() && !cov.cover(m.getPath().string(), m.getFirst().line)) {
        MutationResult result = evaluator.compare(m, actualDir.string(), "uncovered");
        ws.setDone(id, result);
        statusLine.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                                 m.getFirst().line);
        statusLine.recordResult(static_cast<int>(result.getMutationState()));
        continue;
      }

      // Mark as in-progress
      ws.setLock(id);

      statusLine.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                               m.getFirst().line);

      // Mutate
      std::stringstream buf;
      buf << m;
      logger->verbose(fmt::format("Applying: {}", buf.str()));
      repo->getSourceTree()->modify(m, ws.getBackupDir().string());

      // Build
      logger->verbose(fmt::format("Building mutant #{}...", id));
      Subprocess buildProc(cmdPrefix + buildCmd, 0, 0, (ws.getMutantDir(id) / "build.log").string(), silent);
      buildProc.execute();
      std::string testState = "success";
      bool buildSuccess = buildProc.isSuccessfulExit();

      if (buildSuccess) {
        // Test
        logger->verbose("Build OK, running tests...");
        fs::remove_all(testResultDir);
        Subprocess proc(cmdPrefix + testCmd, mTimeLimit, mKillAfter,
                        (ws.getMutantDir(id) / "test.log").string(), silent);
        proc.execute();
        bool testTimeout = proc.isTimedOut();
        if (testTimeout) {
          testState = "timeout";
          logger->warn("Test timed out.");
          fs::remove_all(actualDir);
          fs::remove_all(testResultDir);
        } else {
          copyTestReportTo(testResultDir.string(), actualDir.string(), testResultFileExts);
          fs::remove_all(testResultDir);
        }
      } else {
        testState = "build_failure";
        logger->verbose("Build failed.");
      }

      // Evaluate
      MutationResult result = evaluator.compare(m, actualDir.string(), testState);

      // Restore source
      logger->verbose("Restoring source...");
      restoreBackup(ws.getBackupDir().string(), sourceRoot.string());

      // Mark as complete
      ws.clearLock(id);
      ws.setDone(id, result);
      statusLine.recordResult(static_cast<int>(result.getMutationState()));

      // Clear temp actual dir
      fs::remove_all(actualDir);
    }

    // ── STEP 6: Report ──────────────────────────────────────────────────────────

    statusLine.setPhase(StatusLine::Phase::REPORT);
    fs::create_directories(outputDir);
    outputDir = fs::canonical(outputDir);

    if (!emptyOutputDir) {
      logger->info(fmt::format("Writing report to {}...", outputDir.string()));
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.save(outputDir);
      sentinel::HTMLReport htmlReport(evaluator.getMutationResults(), sourceRoot);
      htmlReport.save(outputDir);
      htmlReport.printSummary();
    } else {
      sentinel::XMLReport xmlReport(evaluator.getMutationResults(), sourceRoot);
      xmlReport.printSummary();
    }
  } catch (...) {
    std::raise(SIGUSR1);
    throw;
  }

  statusLine.disable();
  gStatusLineForSH = nullptr;
  std::raise(SIGUSR1);
  return 0;
}

void CommandRun::copyTestReportTo(const std::string& from, const std::string& to,
                                  const std::vector<std::string>& exts) {
  namespace fs = std::experimental::filesystem;

  fs::remove_all(to);
  fs::create_directories(to);

  if (fs::exists(from) && fs::is_directory(from)) {
    for (const auto& dirent : fs::recursive_directory_iterator(from)) {
      const auto& curPath = dirent.path();
      std::string curExt = curPath.extension().string();
      std::transform(curExt.begin(), curExt.end(), curExt.begin(), [](unsigned char c) { return std::tolower(c); });
      if (fs::is_regular_file(curPath)) {
        bool copyFlag = false;
        if (exts.empty()) {
          copyFlag = true;
        } else {
          for (const auto& t : exts) {
            std::string tmp("." + t);
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c) { return std::tolower(c); });
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
  namespace fs = std::experimental::filesystem;
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
  if (mSourceRoot) {
    return mSourceRoot.Get();
  }
  if (mConfig && mConfig->sourceRoot) {
    return *mConfig->sourceRoot;
  }
  return mSourceRoot.Get();
}

std::string CommandRun::getBuildDir() {
  if (mBuildDir) {
    return mBuildDir.Get();
  }
  if (mConfig && mConfig->buildDir) {
    return *mConfig->buildDir;
  }
  return mBuildDir.Get();
}

std::string CommandRun::getWorkDir() {
  if (mWorkDir) {
    return mWorkDir.Get();
  }
  if (mConfig && mConfig->workDir) {
    return *mConfig->workDir;
  }
  return mWorkDir.Get();
}

std::string CommandRun::getOutputDir() {
  if (mOutputDir) {
    return mOutputDir.Get();
  }
  if (mConfig && mConfig->outputDir) {
    return *mConfig->outputDir;
  }
  return mOutputDir.Get();
}

std::string CommandRun::getTestResultDir() {
  if (mTestResultDir) {
    return mTestResultDir.Get();
  }
  if (mConfig && mConfig->testResultDir) {
    return *mConfig->testResultDir;
  }
  return mTestResultDir.Get();
}

std::string CommandRun::getCompileDbDir() {
  if (mCompileDbDir) {
    return mCompileDbDir.Get();
  }
  if (mConfig && mConfig->compileDbDir) {
    return *mConfig->compileDbDir;
  }
  return mCompileDbDir.Get();
}

std::string CommandRun::getBuildCmd() {
  if (mBuildCmd) {
    return mBuildCmd.Get();
  }
  if (mConfig && mConfig->buildCmd) {
    return *mConfig->buildCmd;
  }
  return mBuildCmd.Get();
}

std::string CommandRun::getTestCmd() {
  if (mTestCmd) {
    return mTestCmd.Get();
  }
  if (mConfig && mConfig->testCmd) {
    return *mConfig->testCmd;
  }
  return mTestCmd.Get();
}

std::string CommandRun::getGenerator() {
  if (mGenerator) {
    return mGenerator.Get();
  }
  if (mConfig && mConfig->generator) {
    return *mConfig->generator;
  }
  return mGenerator.Get();
}

std::vector<std::string> CommandRun::getTestResultFileExts() {
  if (mTestResultFileExts) {
    return mTestResultFileExts.Get();
  }
  if (mConfig && mConfig->testResultFileExts) {
    return *mConfig->testResultFileExts;
  }
  return mTestResultFileExts.Get();
}

std::vector<std::string> CommandRun::getTargetFileExts() {
  if (mExtensions) {
    return mExtensions.Get();
  }
  if (mConfig && mConfig->extensions) {
    return *mConfig->extensions;
  }
  return mExtensions.Get();
}

std::vector<std::string> CommandRun::getPatterns() {
  if (mPatterns) {
    return mPatterns.Get();
  }
  if (mConfig && mConfig->patterns) {
    return *mConfig->patterns;
  }
  return mPatterns.Get();
}

std::vector<std::string> CommandRun::getExcludePaths() {
  if (mExcludes) {
    return mExcludes.Get();
  }
  if (mConfig && mConfig->excludes) {
    return *mConfig->excludes;
  }
  return mExcludes.Get();
}

std::vector<std::string> CommandRun::getCoverageFiles() {
  if (mCoverageFiles) {
    return mCoverageFiles.Get();
  }
  if (mConfig && mConfig->coverageFiles) {
    return *mConfig->coverageFiles;
  }
  return mCoverageFiles.Get();
}

std::string CommandRun::getScope() {
  if (mScope) {
    return mScope.Get();
  }
  if (mConfig && mConfig->scope) {
    return *mConfig->scope;
  }
  return mScope.Get();
}

size_t CommandRun::getMutantLimit() {
  if (mLimit) {
    return mLimit.Get();
  }
  if (mConfig && mConfig->limit) {
    return *mConfig->limit;
  }
  return mLimit.Get();
}

std::string CommandRun::getTestTimeLimit() {
  if (mTimeLimitStr) {
    return mTimeLimitStr.Get();
  }
  if (mConfig && mConfig->timeLimit) {
    return *mConfig->timeLimit;
  }
  return mTimeLimitStr.Get();
}

std::string CommandRun::getKillAfter() {
  if (mKillAfterStr) {
    return mKillAfterStr.Get();
  }
  if (mConfig && mConfig->killAfter) {
    return *mConfig->killAfter;
  }
  return mKillAfterStr.Get();
}

unsigned CommandRun::getSeed() {
  std::string seedStr;
  if (mSeed) {
    seedStr = mSeed.Get();
  } else if (mConfig && mConfig->seed) {
    return *mConfig->seed;
  } else {
    seedStr = mSeed.Get();
  }
  if (seedStr == "auto") {
    return std::random_device {}();
  }
  return static_cast<unsigned>(std::stoul(seedStr));
}

bool CommandRun::getVerbose() {
  return mIsVerbose.Get();
}

std::vector<std::string> CommandRun::getOperators() {
  if (mOperators) {
    return mOperators.Get();
  }
  if (mConfig && mConfig->operators) {
    return *mConfig->operators;
  }
  return mOperators.Get();
}

}  // namespace sentinel
