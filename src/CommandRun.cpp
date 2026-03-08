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
    std::cerr << fmt::format("Received signal: {}.", strsignal(signum)) << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

// Formats a duration in seconds into a human-readable string.
static std::string formatDuration(double secs) {
  if (secs < 60.0) return fmt::format("~{:.0f}s", secs);
  if (secs < 3600.0)
    return fmt::format("~{:.0f}m {:.0f}s", std::floor(secs / 60.0), std::fmod(secs, 60.0));
  return fmt::format("~{:.0f}h {:.0f}m",
                     std::floor(secs / 3600.0), std::fmod(secs, 3600.0) / 60.0);
}

// Formats an exact elapsed duration (no '~' prefix).
static std::string formatElapsed(double secs) {
  if (secs < 60.0) return fmt::format("{:.1f}s", secs);
  if (secs < 3600.0) {
    int m = static_cast<int>(secs / 60.0);
    double s = secs - m * 60.0;
    return fmt::format("{}m {:.0f}s", m, s);
  }
  int h = static_cast<int>(secs / 3600.0);
  int m = static_cast<int>((secs - h * 3600.0) / 60.0);
  return fmt::format("{}h {}m", h, m);
}

// Validates that the test result directory exists and contains result files.
// In normal mode throws on failure; in dry-run mode returns false instead.
static bool validateTestResultDir(const std::experimental::filesystem::path& dir,
                                   const std::string& dirStr, bool dryRun) {
  namespace fs = std::experimental::filesystem;
  if (!fs::exists(dir)) {
    if (!dryRun)
      throw InvalidArgumentException(fmt::format("The test result path does not exist : {0}", dirStr));
    return false;
  }
  if (!fs::is_directory(dir)) {
    if (!dryRun)
      throw InvalidArgumentException(fmt::format("The test result path is not a directory: {0}", dir.c_str()));
    return false;
  }
  if (fs::is_empty(dir)) {
    if (!dryRun)
      throw InvalidArgumentException(fmt::format("The test result path is empty: {0}", dir.c_str()));
    return false;
  }
  return true;
}

// Prints the dry-run summary to stdout.
static void printDryRunSummary(const std::experimental::filesystem::path& sourceRoot,
                                const std::string& buildCmd, const std::string& testCmd,
                                const std::string& scope, size_t mutantLimit,
                                double baselineSecs, size_t timeLimit,
                                const std::string& timeLimitStr,
                                const std::vector<std::pair<int, Mutant>>& indexedMutants,
                                std::size_t candidateCount, bool verbose,
                                bool buildOK, bool testOK,
                                const std::string& workspaceDir,
                                const std::vector<std::string>& operators) {
  fmt::print("\n=== Sentinel Dry Run ===\n");
  fmt::print("  source-dir:    {}\n", sourceRoot.string());
  fmt::print("  build-command: {}\n", buildCmd);
  fmt::print("  test-command:  {}\n", testCmd);
  fmt::print("  scope:         {}\n", scope);
  fmt::print("  limit:         {}\n",
             mutantLimit == 0 ? std::string("unlimited") : std::to_string(mutantLimit));
  fmt::print("  operators:     {}\n",
             operators.empty() ? std::string("all")
                               : sentinel::string::join(", ", operators));
  fmt::print("  workspace:     {}\n", workspaceDir);
  fmt::print("\n");

  fmt::print("  {}  Original build\n", buildOK ? "[ OK ]" : "[FAIL]");
  if (!buildOK) {
    fmt::print("  [ -- ]  Original tests  (skipped)\n");
    fmt::print("  [ -- ]  Mutants         (skipped)\n");
    fmt::print("\nDry run failed. Fix the issues above before running mutation testing.\n");
    return;
  }

  if (timeLimitStr == "auto") {
    fmt::print("  {}  Original tests  (baseline: {:.1f}s, auto-timeout: {}s)\n",
               testOK ? "[ OK ]" : "[FAIL]", baselineSecs, timeLimit);
  } else {
    fmt::print("  {}  Original tests  (timeout: {}s)\n",
               testOK ? "[ OK ]" : "[FAIL]", timeLimit);
  }
  if (!testOK) {
    fmt::print("  [ -- ]  Mutants         (skipped)\n");
    fmt::print("\nDry run failed. Fix the issues above before running mutation testing.\n");
    return;
  }

  if (indexedMutants.empty()) {
    fmt::print("  [WARN]  Mutants: 0 — nothing to evaluate.\n");
    fmt::print("          Check --scope, --pattern, --extension settings.\n");
  } else if (mutantLimit > 0 && indexedMutants.size() >= mutantLimit) {
    fmt::print("  [ OK ]  Mutants: {} of {} candidates (capped at --limit {})\n",
               indexedMutants.size(), candidateCount, mutantLimit);
  } else {
    fmt::print("  [ OK ]  Mutants: {}{}\n", indexedMutants.size(),
               candidateCount > 0 ? fmt::format(" of {} candidates", candidateCount)
                                  : std::string{});
  }

  if (!indexedMutants.empty() && baselineSecs > 0.0) {
    double estimated = baselineSecs * static_cast<double>(indexedMutants.size());
    fmt::print("          Estimated evaluation time: {}  ({} mutant{} x {:.1f}s baseline)\n",
               formatDuration(estimated), indexedMutants.size(),
               indexedMutants.size() == 1 ? "" : "s", baselineSecs);
  }

  if (verbose) {
    for (const auto& [id, m] : indexedMutants) {
      fmt::print("          [{:3d}] {} @ {}:{}\n", id, m.getOperator(),
                 m.getPath().filename().string(), m.getFirst().line);
    }
  }

  if (!indexedMutants.empty()) {
    fmt::print("\nWorkspace saved. Remove --dry-run to start mutation testing\n");
    fmt::print("(the workspace will be reused — build and populate steps are skipped).\n");
  }
}

static const char* const kYamlTemplate =
    "# sentinel.yaml — full configuration template\n"
    "#\n"
    "# Uncomment and edit the options you need.\n"
    "# CLI arguments always take priority over values in this file.\n"
    "\n"
    "# Directory for output reports (default: none)\n"
    "# output-dir: ./sentinel_output\n"
    "\n"
    "# Workspace directory for all sentinel run artifacts (default: ./sentinel_workspace)\n"
    "# workspace: ./sentinel_workspace\n"
    "\n"
    "# Enable verbose logging (default: false)\n"
    "# verbose: false\n"
    "\n"
    "# Suppress build/test log output to the terminal; status line still shows progress (default: false)\n"
    "# silent: false\n"
    "\n"
    "# Enable debug logging (default: false)\n"
    "# debug: false\n"
    "\n"
    "# --- Run options ---\n"
    "\n"
    "# Disable the terminal status line even when stdout is a TTY (default: false)\n"
    "# no-statusline: false\n"
    "\n"
    "# --- Build & test options ---\n"
    "\n"
    "# Source root directory (default: .)\n"
    "# source-dir: .\n"
    "\n"
    "# Shell command to build the source\n"
    "# build-command: make\n"
    "\n"
    "# Path to directory containing compile_commands.json (default: .)\n"
    "# compiledb-dir: .\n"
    "\n"
    "# Shell command to execute tests\n"
    "# test-command: make test\n"
    "\n"
    "# Path to the test report directory\n"
    "# test-report-dir: ./test-results\n"
    "\n"
    "# File extension of the test report (default: xml)\n"
    "# test-report-extension:\n"
    "#   - xml\n"
    "\n"
    "# Test time limit in seconds (default: auto — 2x baseline run time; 0 = no limit)\n"
    "# timeout: auto\n"
    "\n"
    "# Seconds to wait after timeout before sending SIGKILL (default: 60; 0 = disabled)\n"
    "# kill-after: 60\n"
    "\n"
    "# --- Mutation options ---\n"
    "\n"
    "# Diff scope: 'commit' (changed lines only) or 'all' (entire codebase) (default: all)\n"
    "# scope: all\n"
    "\n"
    "# Source file extensions to mutate (default: cxx cpp cc c c++ cu)\n"
    "# extension:\n"
    "#   - cpp\n"
    "#   - cxx\n"
    "#   - cc\n"
    "#   - c\n"
    "#   - c++\n"
    "#   - cu\n"
    "\n"
    "# Paths or glob patterns to constrain the diff (default: none — entire source)\n"
    "# pattern: []\n"
    "\n"
    "# Paths excluded from mutation; fnmatch-style patterns (default: none)\n"
    "# exclude: []\n"
    "\n"
    "# Maximum number of mutants to generate (default: 10)\n"
    "# limit: 10\n"
    "\n"
    "# Mutant selection strategy (default: uniform)\n"
    "#   uniform  - one mutant per operator per source line\n"
    "#   random   - randomly sampled from all possible mutants\n"
    "#   weighted - samples more mutants from complex code\n"
    "# generator: uniform\n"
    "\n"
    "# Random seed for mutant selection (default: auto — picked randomly)\n"
    "# seed: auto\n"
    "\n"
    "# Mutation operators to use; omit to use all operators (default: all)\n"
    "#   AOR - Arithmetic Operator Replacement  (+, -, *, /)\n"
    "#   BOR - Bitwise Operator Replacement      (&, |, ^)\n"
    "#   LCR - Logical Connector Replacement     (&&, ||)\n"
    "#   ROR - Relational Operator Replacement   (<, >, ==, !=)\n"
    "#   SDL - Statement Deletion\n"
    "#   SOR - Shift Operator Replacement        (<<, >>)\n"
    "#   UOI - Unary Operator Insertion          (-x, !x)\n"
    "# operator:\n"
    "#   - AOR\n"
    "#   - BOR\n"
    "#   - LCR\n"
    "#   - ROR\n"
    "#   - SDL\n"
    "#   - SOR\n"
    "#   - UOI\n"
    "\n"
    "# lcov-format coverage result files; limits mutation to covered lines only (default: none)\n"
    "# coverage: []\n";

CommandRun::CommandRun(args::Group& parser) :
    Command(parser),
    mGroupRunCtrl(parser, "Run options:"),
    mGroupBuildTest(parser, "Build & test options:"),
    mGroupMutation(parser, "Mutation options:"),
    mConfigFile(mGroupRunCtrl, "PATH", "Path to a YAML config file (default: sentinel.yaml in current directory)",
                {"config"}),
    mInit(mGroupRunCtrl, "init", "Write a sentinel.yaml config template to the current directory and exit",
          {"init"}),
    mDryRun(mGroupRunCtrl, "dry-run",
            "Build, test, and generate mutants then exit; prints a readiness summary without evaluating mutants",
            {"dry-run"}),
    mNoStatusLine(mGroupRunCtrl, "no-statusline", "Disable the live status line shown in TTY mode",
                  {"no-statusline"}),
    mSourceDir(mGroupBuildTest, "PATH", "Path to the root of the source tree.", {"source-dir"}, "."),
    mBuildCmd(mGroupBuildTest, "CMD", "Shell command to build the project [required]", {"build-command"}),
    mCompileDbDir(mGroupBuildTest, "PATH", "Path to the directory containing compile_commands.json.",
                  {"compiledb-dir"}, "."),
    mTestCmd(mGroupBuildTest, "CMD", "Shell command to run tests [required]", {"test-command"}),
    mTestResultDir(mGroupBuildTest, "PATH", "Path to the test report directory [required]",
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
    mGenerator(mGroupMutation, "TYPE",
               "Mutant selection strategy: uniform=one per operator per line, "
               "random=fully random, weighted=complex-code-first (default: uniform)",
               {"generator"}, "uniform"),
    mSeed(mGroupMutation, "N", "Random seed for mutant selection ('auto' = pick randomly)", {"seed"}, "auto"),
    mOperators(mGroupMutation, "OP",
               "Mutation operators to apply (default: all). "
               "AOR=Arithmetic BOR=Bitwise LCR=Logical ROR=Relational SDL=StmtDel SOR=Shift UOI=Unary",
               {"operator"}),
    mCoverageFiles(mGroupMutation, "FILE",
                   "lcov coverage info file; limits mutation to covered lines only", {"coverage"}) {}

void CommandRun::init() {
  namespace fs = std::experimental::filesystem;

  if (!mInit) {
    // Determine config file path.
    std::string configPath;
    if (mConfigFile) {
      configPath = mConfigFile.Get();
      if (!fs::exists(configPath)) {
        throw std::runtime_error(fmt::format("Config file not found: {}", configPath));
      }
    } else if (fs::exists("sentinel.yaml")) {
      configPath = "sentinel.yaml";
    }

    // Load YAML config if a file was found or explicitly specified,
    // then chdir to the config file's directory so that all relative paths
    // in sentinel.yaml and subsequent CLI options share a consistent base.
    if (!configPath.empty()) {
      mConfig = SentinelConfig::loadFromFile(configPath);
      fs::path configDir = fs::absolute(fs::path(configPath)).parent_path();

      // Before changing directory, resolve any CLI-provided relative paths to
      // absolute paths so they remain valid after the chdir.
      if (configDir != fs::current_path()) {
        mChangedToDir = configDir.string();
        auto toAbs = [](args::ValueFlag<std::string>& flag) {
          if (flag) {
            fs::path p(flag.Get());
            if (p.is_relative()) {
              flag.Get() = fs::absolute(p).string();
            }
          }
        };
        toAbs(mOutputDir);
        toAbs(mWorkDir);
        toAbs(mSourceDir);
        toAbs(mCompileDbDir);
        toAbs(mTestResultDir);
        if (mCoverageFiles) {
          for (auto& f : mCoverageFiles.Get()) {
            fs::path p(f);
            if (p.is_relative()) {
              f = fs::absolute(p).string();
            }
          }
        }
      }

      std::error_code ec;
      fs::current_path(configDir, ec);
      if (ec) {
        throw std::runtime_error(
            fmt::format("Failed to change directory to '{}': {}", configDir.string(), ec.message()));
      }
    }
  }

  // Call base init: sets log level from CLI -v/--debug.
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
static std::string buildWorkspaceYaml(const std::string& sourceRoot,
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
  out << YAML::Key << "source-dir" << YAML::Value << sourceRoot;
  out << YAML::Key << "verbose" << YAML::Value << verbose;
  out << YAML::Key << "debug" << YAML::Value << debug;
  out << YAML::Key << "no-statusline" << YAML::Value << noStatusLine;
  out << YAML::Key << "silent" << YAML::Value << silent;
  if (!outputDir.empty()) {
    out << YAML::Key << "output-dir" << YAML::Value << outputDir;
  }
  out << YAML::Key << "compiledb-dir" << YAML::Value << compileDbDir;
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
      bool overwrite = mForce.Get();
      if (!overwrite) {
        fmt::print("'{}' already exists. Overwrite? [y/N] ", kConfigFileName);
        std::string answer;
        std::getline(std::cin, answer);
        overwrite = (answer == "y" || answer == "Y");
      }
      if (!overwrite) {
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

  bool dryRun = static_cast<bool>(mDryRun);

  StatusLine statusLine;
  statusLine.setDryRun(dryRun);
  gStatusLineForSH = &statusLine;

  // ── STEP 1: Workspace setup ─────────────────────────────────────────────────

  fs::path workDirPath = fs::absolute(getWorkDir());
  Workspace ws(workDirPath);

  bool resuming = false;
  SentinelConfig activeConfig;

  if (ws.hasPreviousRun()) {
    bool resume = !mForce.Get();
    if (resume) {
      fmt::print("Previous run found in '{}'. Resume? [y/N] ", workDirPath.string());
      std::string answer;
      std::getline(std::cin, answer);
      resume = (answer == "y" || answer == "Y");
    }
    if (resume) {
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
    sourceRootStr = activeConfig.sourceDir.value_or(".");
    compileDbStr = activeConfig.compileDbDir.value_or(".");
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
    sourceRootStr = getSourceDir();
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
    throw InvalidArgumentException(
        "Option --build-command is required.\n"
        "  hint: pass --build-command 'make' or add 'build-command: make' to sentinel.yaml");
  }
  if (testCmd.empty()) {
    throw InvalidArgumentException(
        "Option --test-command is required.\n"
        "  hint: pass --test-command 'ctest' or add 'test-command: ctest' to sentinel.yaml");
  }
  if (testResultDirStr.empty()) {
    throw InvalidArgumentException(
        "Option --test-report-dir is required.\n"
        "  hint: pass --test-report-dir <path> or add 'test-report-dir: <path>' to sentinel.yaml");
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
            "Invalid timeout value: '{}'. Expected a positive integer (seconds) or 'auto'.",
            timeLimitStr));
      }
    }
  }

  // Canonicalize paths
  fs::path sourceRoot = fs::canonical(sourceRootStr);
  sourceRootForSH = sourceRoot;
  backupDirForSH = ws.getBackupDir();
  setSignalHandler();

  compileDbStr = fs::absolute(compileDbStr).string();

  // testResultDir: validation in fresh mode
  if (!resuming) {
    if (fs::exists(testResultDirStr)) {
      if (!fs::is_directory(testResultDirStr)) {
        throw InvalidArgumentException(
            fmt::format("The given test result path is not a directory: {0}", testResultDirStr));
      }
      if (!fs::is_empty(testResultDirStr)) {
        bool deleteDir = mForce.Get();
        if (!deleteDir) {
          fmt::print("The given test result path is not empty: {0}\nDelete and continue? [y/N] ", testResultDirStr);
          std::string answer;
          std::getline(std::cin, answer);
          deleteDir = (answer == "y" || answer == "Y");
        }
        if (!deleteDir) {
          throw InvalidArgumentException(
              fmt::format("The given test result path is not empty: {0}", testResultDirStr));
        }
        fs::remove_all(testResultDirStr);
      }
    }
  }
  fs::path testResultDir = fs::absolute(testResultDirStr);

  bool emptyOutputDir = outputDirStr.empty();
  fs::path outputDir = emptyOutputDir ? fs::absolute(".") : fs::absolute(outputDirStr);

  // ── Pre-run configuration validation ────────────────────────────────────────
  // Collect potentially problematic settings and ask the user to confirm before
  // starting the (potentially hours-long) pipeline.
  if (!resuming) {
    // Source labels: distinguish CLI flags from sentinel.yaml keys.
    const std::string limitSrc   = mLimit        ? "--limit (CLI)"   : "limit (sentinel.yaml)";
    const std::string timeoutSrc = mTimeLimitStr  ? "--timeout (CLI)" : "timeout (sentinel.yaml)";
    const std::string patternSrc = mPatterns      ? "--pattern (CLI)" : "pattern (sentinel.yaml)";
    const std::string excludeSrc = mExcludes      ? "--exclude (CLI)" : "exclude (sentinel.yaml)";

    std::vector<std::string> warnings;

    // Working-directory change caused by --config pointing to a different directory.
    if (mChangedToDir) {
      warnings.push_back(fmt::format(
          "--config (CLI): working directory changed to '{}'. "
          "All relative paths and commands run from that location.",
          *mChangedToDir));
    }

    // limit=0: evaluates every candidate mutant, which may take hours.
    if (mutantLimit == 0) {
      warnings.push_back(fmt::format(
          "{}: 0 — all candidate mutants will be evaluated. "
          "This may take a very long time.",
          limitSrc));
    }

    // timeout=0: no per-mutant time cap; a hanging test blocks the run forever.
    if (timeLimitStr == "0") {
      warnings.push_back(fmt::format(
          "{}: 0 — no per-mutant test time limit. "
          "A hanging test will block the run indefinitely.",
          timeoutSrc));
    }

    // --pattern checks (git pathspec, matched against repo-relative paths).
    for (const auto& pat : diffPatterns) {
      fs::path p(pat);
      if (p.is_absolute()) {
        auto mm = std::mismatch(sourceRoot.begin(), sourceRoot.end(), p.begin(), p.end());
        if (mm.first != sourceRoot.end()) {
          // Absolute path outside the source root: likely a different repository.
          warnings.push_back(fmt::format(
              "{}: '{}' is an absolute path outside source-dir '{}'. "
              "It likely points to a different repository and will produce no mutants.",
              patternSrc, pat, sourceRoot.string()));
        } else {
          // Absolute path inside the source root: git pathspec expects repo-relative paths.
          warnings.push_back(fmt::format(
              "{}: '{}' is an absolute path. "
              "Git pathspec uses paths relative to the repository root; "
              "consider using a relative path instead.",
              patternSrc, pat));
        }
      }
    }

    // --exclude checks (fnmatch, matched against canonical absolute file paths).
    for (const auto& excl : excludePaths) {
      fs::path p(excl);
      if (p.is_absolute()) {
        auto mm = std::mismatch(sourceRoot.begin(), sourceRoot.end(), p.begin(), p.end());
        if (mm.first != sourceRoot.end()) {
          // Absolute path outside the source root: will never match a source file.
          warnings.push_back(fmt::format(
              "{}: '{}' is an absolute path outside source-dir '{}'. "
              "It likely points to a different repository and will never match any source file.",
              excludeSrc, excl, sourceRoot.string()));
        }
      } else if (!excl.empty() && excl.back() == '/') {
        // Trailing slash: fnmatch matches against file paths, which never end with '/'.
        warnings.push_back(fmt::format(
            "{}: '{}' ends with '/'. "
            "Patterns are matched against file paths, not directories. "
            "Did you mean '{}*'?",
            excludeSrc, excl, excl));
      } else if (!excl.empty() && excl.front() != '*') {
        // Relative path without a leading wildcard: fnmatch matches against the full
        // absolute path, so a bare name like 'build' never matches '/abs/path/build/foo.cpp'.
        warnings.push_back(fmt::format(
            "{}: '{}' is a relative pattern without a leading '*'. "
            "Patterns are matched against absolute file paths. "
            "Did you mean '*/{}/*'?",
            excludeSrc, excl, excl));
      }
    }

    if (!warnings.empty() && !mForce.Get()) {
      std::cout << "\nConfiguration warnings:\n";
      for (const auto& w : warnings) {
        std::cout << fmt::format("  [!] {}\n", w);
      }
      std::cout << "\nProceed? [y/N] ";
      std::cout.flush();
      std::string answer;
      std::getline(std::cin, answer);
      if (answer != "y" && answer != "Y") {
        std::cout << "Aborted.\n";
        return 0;
      }
    }
  }

  auto logger = Logger::getLogger(cCommandRunLoggerName);

  if (resuming) {
    logger->warn(fmt::format("Resuming with saved config: {}. "
                             "New CLI options and sentinel.yaml changes are ignored.",
                             (ws.getRoot() / "sentinel.yaml").string()));
  }

  logger->verbose(fmt::format("source root:        {}", sourceRoot.string()));
  logger->verbose(fmt::format("build cmd:          {}", buildCmd));
  logger->verbose(fmt::format("compiledb dir:      {}", compileDbStr));
  logger->verbose(fmt::format("test cmd:           {}", testCmd));
  logger->verbose(fmt::format("test result dir:    {}", testResultDir.string()));
  logger->verbose(fmt::format("test result exts:   {}", sentinel::string::join(", ", testResultFileExts)));
  logger->verbose(fmt::format("timeout:            {}s", mTimeLimit));
  logger->verbose(fmt::format("kill-after:         {}s", mKillAfter));
  logger->verbose(fmt::format("scope:              {}", scope));
  logger->verbose(fmt::format("source extensions:  {}", sentinel::string::join(", ", targetFileExts)));
  logger->verbose(fmt::format("patterns:           {}", sentinel::string::join(", ", diffPatterns)));
  logger->verbose(fmt::format("exclude patterns:   {}", sentinel::string::join(", ", excludePaths)));
  logger->verbose(fmt::format("mutant limit:       {}", mutantLimit));
  logger->verbose(fmt::format("generator:          {}", generatorStr));
  logger->verbose(fmt::format("random seed:        {}", randomSeed));
  logger->verbose(fmt::format("coverage files:     {}", sentinel::string::join(", ", coverageFiles)));
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


  for (const auto& filename : coverageFiles) {
    if (!fs::exists(filename)) {
      throw InvalidArgumentException(fmt::format("Input coverage file does not exist: {}", filename));
    }
  }

  double baselineSecs = 0.0;
  bool dryRunBuildOK = true;
  bool dryRunTestOK = true;

  try {
    // ── STEP 3: Original build & test ──────────────────────────────────────────
    // Skip if resuming and original results already exist.
    bool origDone = resuming && fs::exists(ws.getOriginalDir() / "build.log") &&
                    fs::exists(ws.getOriginalResultsDir()) && !fs::is_empty(ws.getOriginalResultsDir());

    if (!origDone) {
      // build
      logger->info("Building original source code...");
      auto buildPhaseStart = std::chrono::steady_clock::now();
      Subprocess origBuildProc(buildCmd, 0, 0, (ws.getOriginalDir() / "build.log").string(), silent);
      origBuildProc.execute();
      double buildElapsed = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - buildPhaseStart).count();
      if (!origBuildProc.isSuccessfulExit()) {
        logger->info(fmt::format("Build failed ({}).", formatElapsed(buildElapsed)));
        if (!dryRun) throw std::runtime_error("Build failed.");
        dryRunBuildOK = false;
      } else {
        logger->info(fmt::format("Build done ({}).", formatElapsed(buildElapsed)));
      }

      if (!dryRun || dryRunBuildOK) {
        // test
        logger->info("Running original tests...");
        statusLine.setPhase(StatusLine::Phase::TEST_ORIG);
        fs::remove_all(testResultDir);
        Subprocess firstTestProc(testCmd, mTimeLimit, mKillAfter,
                                 (ws.getOriginalDir() / "test.log").string(), silent);

        auto start = std::chrono::steady_clock::now();
        firstTestProc.execute();
        double testElapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();

        if (!resuming && timeLimitStr == "auto") {
          baselineSecs = testElapsed;
          if (testElapsed < 1.0) {
            mTimeLimit = 1;
          } else {
            mTimeLimit = static_cast<size_t>(ceil(testElapsed * 2.0));
          }
          logger->info(fmt::format("Auto timeout: {}s (baseline {:.1f}s x2)", mTimeLimit, testElapsed));
        }

        if (firstTestProc.isTimedOut()) {
          logger->info(fmt::format("Tests timed out ({}).", formatElapsed(testElapsed)));
          if (!dryRun) throw std::runtime_error("Timeout occurs when excuting test cmd for original source code.");
          dryRunTestOK = false;
        }
        if (dryRunTestOK && !validateTestResultDir(testResultDir, testResultDirStr, dryRun)) {
          dryRunTestOK = false;
        }

        if (dryRunTestOK) {
          logger->info(fmt::format("Tests done ({}).", formatElapsed(testElapsed)));
          // Copy baseline test results to workspace/original/results/
          copyTestReportTo(testResultDir.string(), ws.getOriginalResultsDir().string(), testResultFileExts);

          // Save workspace config now that the baseline phase is complete.
          // From this point on, hasPreviousRun() will return true and resume is possible.
          ws.saveConfig(buildWorkspaceYaml(sourceRoot.string(), compileDbStr, scope, targetFileExts,
                                           diffPatterns, excludePaths, mutantLimit,
                                           buildCmd, testCmd, testResultDir.string(),
                                           testResultFileExts, coverageFiles, generatorStr,
                                           mTimeLimit, mKillAfter, randomSeed, operators,
                                           emptyOutputDir ? std::string{} : outputDir.string(),
                                           mIsVerbose.Get(), mIsDebug.Get(),
                                           disableStatusLine, silent));
        }
      }  // end if (!dryRun || dryRunBuildOK)
    }

    // Restore any leftover backup from a previously interrupted run
    restoreBackup(ws.getBackupDir().string(), sourceRoot.string());

    // ── STEP 4: Populate or reload mutants ─────────────────────────────────────

    std::vector<std::pair<int, Mutant>> indexedMutants;
    std::size_t candidateCount = 0;

    if (resuming) {
      indexedMutants = ws.loadMutants();
      logger->info(fmt::format("Loaded {} mutants from workspace.", indexedMutants.size()));
      statusLine.setTotalMutants(indexedMutants.size());
    } else if (!dryRun || (dryRunBuildOK && dryRunTestOK)) {
      statusLine.setPhase(StatusLine::Phase::POPULATE);
      logger->info("Generating mutants...");
      auto populateStart = std::chrono::steady_clock::now();
      auto repo =
          std::make_unique<sentinel::GitRepository>(sourceRoot, targetFileExts, diffPatterns, excludePaths);
      repo->addSkipDir(workDirPath);
      sentinel::SourceLines sourceLines = repo->getSourceLines(scope);
      if (sourceLines.empty()) {
        if (scope == "commit") {
          logger->warn("No changed source lines found (scope=commit). "
                       "Make sure there are uncommitted modifications in the source tree. "
                       "Use --scope all to mutate the entire codebase.");
        } else {
          logger->warn("No source lines found to mutate. "
                       "Check --source-dir, --pattern, and --extension options.");
        }
      }
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
      candidateCount = generator->getCandidateCount();

      int id = 1;
      for (auto& m : mutants) {
        logger->verbose(fmt::format("mutant: {}", m.str()));
        ws.createMutant(id, m);
        indexedMutants.emplace_back(id, m);
        id++;
      }
      statusLine.setTotalMutants(indexedMutants.size());
      double populateElapsed = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - populateStart).count();
      logger->info(fmt::format("Generated {} mutant{} ({}).",
                               indexedMutants.size(), indexedMutants.size() == 1 ? "" : "s",
                               formatElapsed(populateElapsed)));
      if (mutantLimit > 0 && indexedMutants.size() >= mutantLimit) {
        std::cout << fmt::format("Note: mutant count capped at {} of {} candidates (--limit {}).\n",
                                 mutantLimit, candidateCount, mutantLimit);
      }
    }

    // ── DRY-RUN EXIT ────────────────────────────────────────────────────────────

    if (dryRun) {
      statusLine.disable();
      gStatusLineForSH = nullptr;
      printDryRunSummary(sourceRoot, buildCmd, testCmd, scope, mutantLimit,
                         baselineSecs, mTimeLimit, timeLimitStr,
                         indexedMutants, candidateCount, getVerbose(),
                         dryRunBuildOK, dryRunTestOK, workDirPath.string(), operators);
      std::raise(SIGUSR1);
      return (dryRunBuildOK && dryRunTestOK) ? 0 : 1;
    }

    // ── STEP 5: Mutant loop ─────────────────────────────────────────────────────

    fs::path actualDir = ws.getRoot() / "actual";
    auto repo = std::make_unique<sentinel::GitRepository>(sourceRoot, targetFileExts, diffPatterns, excludePaths);
    repo->addSkipDir(workDirPath);
    sentinel::Evaluator evaluator(ws.getOriginalResultsDir().string(), sourceRoot.string());
    CoverageInfo cov(coverageFiles);

    statusLine.setPhase(StatusLine::Phase::MUTANT);

    size_t totalMutants = indexedMutants.size();
    logger->info(fmt::format("Evaluating {} mutant{}...", totalMutants, totalMutants == 1 ? "" : "s"));
    auto evalStart = std::chrono::steady_clock::now();

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

      // Coverage check
      if (!coverageFiles.empty() && !cov.cover(m.getPath().string(), m.getFirst().line)) {
        MutationResult result = evaluator.compare(m, actualDir.string(), "uncovered");
        ws.setDone(id, result);
        statusLine.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                                 m.getFirst().line);
        statusLine.recordResult(static_cast<int>(result.getMutationState()));
        logger->info(fmt::format("[{}/{}] {} @ {}:{} → {}",
                                 id, totalMutants,
                                 m.getOperator(), m.getPath().filename().string(), m.getFirst().line,
                                 MutationStateToStr(result.getMutationState())));
        continue;
      }

      // Mark as in-progress
      ws.setLock(id);

      statusLine.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                               m.getFirst().line);

      auto mutantStart = std::chrono::steady_clock::now();

      // Mutate
      std::stringstream buf;
      buf << m;
      logger->verbose(fmt::format("Applying: {}", buf.str()));
      repo->getSourceTree()->modify(m, ws.getBackupDir().string());

      // Build
      logger->verbose(fmt::format("Building mutant #{}...", id));
      Subprocess buildProc(buildCmd, 0, 0, (ws.getMutantDir(id) / "build.log").string(), silent);
      buildProc.execute();
      std::string testState = "success";
      bool buildSuccess = buildProc.isSuccessfulExit();

      if (buildSuccess) {
        // Test
        logger->verbose("Build OK, running tests...");
        fs::remove_all(testResultDir);
        Subprocess proc(testCmd, mTimeLimit, mKillAfter,
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
      double mutantElapsed = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - mutantStart).count();

      // Restore source
      logger->verbose("Restoring source...");
      restoreBackup(ws.getBackupDir().string(), sourceRoot.string());

      // Mark as complete
      ws.clearLock(id);
      ws.setDone(id, result);
      statusLine.recordResult(static_cast<int>(result.getMutationState()));

      // Per-mutant result line (always visible at INFO level)
      logger->info(fmt::format("[{}/{}] {} @ {}:{} → {}  ({})",
                               id, totalMutants,
                               m.getOperator(), m.getPath().filename().string(), m.getFirst().line,
                               MutationStateToStr(result.getMutationState()),
                               formatElapsed(mutantElapsed)));

      // Clear temp actual dir
      fs::remove_all(actualDir);
    }

    double evalElapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - evalStart).count();
    logger->info(fmt::format("Evaluation done ({}).", formatElapsed(evalElapsed)));

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
      throw InvalidArgumentException(fmt::format("'{}' must be a directory.", target));
    }

    if (!isFilledDir && !fs::is_empty(target)) {
      throw InvalidArgumentException(fmt::format("'{}' must be empty.", target));
    }
  }
  return fs::canonical(target).string();
}

std::string CommandRun::getSourceDir() {
  if (mSourceDir) {
    return mSourceDir.Get();
  }
  if (mConfig && mConfig->sourceDir) {
    return *mConfig->sourceDir;
  }
  return mSourceDir.Get();
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
