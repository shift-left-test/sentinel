/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <exception>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/CommandRun.hpp"
#include "sentinel/Console.hpp"
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SentinelConfig.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/TimeStamper.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/signal.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::filesystem;

namespace sentinel {

static const char* cCommandRunLoggerName = "CommandRun";
static fs::path backupDirForSH;
static fs::path sourceRootForSH;
static fs::path workspaceDirForSH;
static StatusLine* gStatusLineForSH = nullptr;

static void signalHandler(int signum) {
  if (!backupDirForSH.empty() && fs::exists(backupDirForSH) && fs::is_directory(backupDirForSH)) {
    CommandRun::restoreBackup(backupDirForSH.string(), sourceRootForSH.string());
  }
  if (gStatusLineForSH != nullptr) {
    gStatusLineForSH->disable();
    gStatusLineForSH = nullptr;
  }
  Console::flush();
  if (signum != SIGUSR1) {
    Console::err("Received signal: {}.", strsignal(signum));
    if (!workspaceDirForSH.empty()) {
      Console::err("  hint: Check logs in {} for details.", workspaceDirForSH.string());
    }
    std::exit(EXIT_FAILURE);
  }
}

// Validates that the test result directory exists and contains result files.
// In normal mode throws on failure; in dry-run mode returns false instead.
static bool validateTestResultDir(const std::filesystem::path& dir,
                                  const std::string& dirStr, bool dryRun) {
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
static void printDryRunSummary(const std::filesystem::path& sourceRoot,
                               const std::string& buildCmd, const std::string& testCmd,
                               const std::string& scope, size_t mutantLimit,
                               double baselineSecs, size_t timeLimit,
                               const std::string& timeLimitStr,
                               const std::vector<std::pair<int, Mutant>>& indexedMutants,
                               std::size_t candidateCount, bool verbose,
                               bool buildOK, bool testOK,
                               const std::string& workspaceDir,
                               const std::vector<std::string>& operators,
                               size_t partIdx, size_t partCount,
                               size_t fullMutantCount) {
  Console::out("\n=== Sentinel Dry Run ===");
  Console::out("  source-dir:    {}", sourceRoot.string());
  Console::out("  build-command: {}", buildCmd);
  Console::out("  test-command:  {}", testCmd);
  Console::out("  scope:         {}", scope);
  Console::out("  limit:         {}", mutantLimit == 0 ? "unlimited" : std::to_string(mutantLimit));
  Console::out("  operators:     {}", operators.empty() ? "all" : sentinel::string::join(", ", operators));
  if (partIdx != 0) {
    Console::out("  partition:     {}/{}", partIdx, partCount);
  }
  Console::out("  workspace:     {}", workspaceDir);

  Console::out("\n  {}  Original build", buildOK ? "[ OK ]" : "[FAIL]");
  if (!buildOK) {
    Console::out("  [ -- ]  Original tests  (skipped)");
    Console::out("  [ -- ]  Mutants         (skipped)");
    Console::out("\nDry run failed. Fix the issues above before running mutation testing.");
    return;
  }

  if (timeLimitStr == "auto") {
    Console::out("  {}  Original tests  (baseline: {:.1f}s, auto-timeout: {}s)",
                 testOK ? "[ OK ]" : "[FAIL]", baselineSecs, timeLimit);
  } else {
    Console::out("  {}  Original tests  (timeout: {}s)", testOK ? "[ OK ]" : "[FAIL]", timeLimit);
  }
  if (!testOK) {
    Console::out("  [ -- ]  Mutants         (skipped)");
    Console::out("\nDry run failed. Fix the issues above before running mutation testing.");
    return;
  }

  if (indexedMutants.empty()) {
    Console::out("  [WARN]  Mutants: 0 — nothing to evaluate.");
    if (partIdx != 0) {
      Console::out("          (partition {}/{} of {} total mutants)", partIdx, partCount, fullMutantCount);
    }
    Console::out("          Check --scope, --pattern, --extension settings.");
  } else if (partIdx != 0) {
    Console::out("  [ OK ]  Mutants: {} (partition {}/{} of {} total)",
                 indexedMutants.size(), partIdx, partCount, fullMutantCount);
  } else if (mutantLimit > 0 && indexedMutants.size() >= mutantLimit) {
    Console::out("  [ OK ]  Mutants: {} of {} candidates (capped at --limit {})",
                 indexedMutants.size(), candidateCount, mutantLimit);
  } else {
    Console::out("  [ OK ]  Mutants: {}{}", indexedMutants.size(),
                 candidateCount > 0 ? fmt::format(" of {} candidates", candidateCount) : "");
  }

  if (verbose) {
    for (const auto& [id, m] : indexedMutants) {
      Console::out("          [{:3d}] {} @ {}:{}", id, m.getOperator(),
                   m.getPath().filename().string(), m.getFirst().line);
    }
  }

  if (!indexedMutants.empty()) {
    Console::out("\nWorkspace saved. Remove --dry-run to start mutation testing");
    Console::out("(the workspace will be reused — build and populate steps are skipped).");
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
    "# --- Run options ---\n"
    "\n"
    "# Fail with exit code 3 if mutation score is below this percentage 0–100 (default: disabled)\n"
    "# threshold: 80\n"
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
    "# Maximum number of mutants to generate (default: 0 = unlimited)\n"
    "# limit: 0\n"
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
    mThreshold(mGroupRunCtrl, "PCT",
               "Fail with exit code 3 if mutation score is below this percentage (0–100)",
               {"threshold"}),
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
    mLimit(mGroupMutation, "N", "Maximum number of mutants to generate (default: 0 = unlimited)",
           {'l', "limit"}, 0),
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
                   "lcov coverage info file; limits mutation to covered lines only", {"coverage"}),
    mPartition(mGroupMutation, "N/TOTAL",
               "Evaluate only the N-th slice of the full mutant list out of TOTAL "
               "partitions (1-based, e.g., --partition=2/5). "
               "Requires --seed to be explicitly set so every partition generates an "
               "identical mutant list. Combine all partition results to obtain the same "
               "outcome as a single non-partitioned run.",
               {"partition"}) {}

void CommandRun::init() {
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

  // Call base init: sets log level from CLI --verbose/--debug.
  Command::init();
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
                                      const std::string& outputDir) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "source-dir" << YAML::Value << sourceRoot;
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

namespace {

struct RunCfg {
  std::filesystem::path sourceRoot;
  std::filesystem::path workDirPath;
  std::string buildCmd;
  std::string testCmd;
  std::filesystem::path testResultDir;
  std::string testResultDirStr;
  std::vector<std::string> testResultFileExts;
  std::vector<std::string> coverageFiles;
  std::string scope;
  std::string generatorStr;
  size_t mutantLimit = 0;
  size_t timeLimit = 0;
  size_t killAfter = 0;
  unsigned randomSeed = 0;
  std::vector<std::string> targetFileExts;
  std::vector<std::string> excludePaths;
  std::vector<std::string> diffPatterns;
  std::vector<std::string> operators;
  std::filesystem::path outputDir;
  bool emptyOutputDir = true;
  bool silent = false;
  std::string compileDbStr;
  std::string timeLimitStr;
  bool resuming = false;
  bool dryRun = false;
  bool disableStatusLine = false;
  std::optional<double> threshold;
};

struct BaselineResult {
  double baselineSecs = 0.0;
  bool buildOK = true;
  bool testOK = true;
};

struct PopulateResult {
  std::vector<std::pair<int, sentinel::Mutant>> indexedMutants;
  std::size_t candidateCount = 0;
};

}  // namespace

static BaselineResult runBaselineBuildAndTest(RunCfg& cfg,                           // NOLINT
                                              sentinel::Workspace& ws,               // NOLINT
                                              sentinel::StatusLine& sl,              // NOLINT
                                              const std::shared_ptr<sentinel::Logger>& logger) {
  BaselineResult r;
  TimeStamper timeStamper;

  sl.setPhase(sentinel::StatusLine::Phase::BUILD_ORIG);

  bool origDone = cfg.resuming && fs::exists(ws.getOriginalDir() / "build.log") &&
                  fs::exists(ws.getOriginalResultsDir()) && !fs::is_empty(ws.getOriginalResultsDir());
  if (origDone) {
    return r;
  }

  logger->info("Building original source code...");
  timeStamper.reset();
  sentinel::Subprocess origBuildProc(cfg.buildCmd, 0, 0,
                                     (ws.getOriginalDir() / "build.log").string(), cfg.silent);
  origBuildProc.execute();
  if (!origBuildProc.isSuccessfulExit()) {
    logger->info("Build failed ({}).", timeStamper.toString(TimeStamper::Format::HumanReadable));
    logger->info("  hint: See {} for details.", (ws.getOriginalDir() / "build.log").string());
    if (!cfg.dryRun) throw std::runtime_error("Build failed.");
    r.buildOK = false;
  } else {
    logger->info("Build done ({}).", timeStamper.toString(TimeStamper::Format::HumanReadable));
  }

  if (!cfg.dryRun || r.buildOK) {
    logger->info("Running original tests...");
    sl.setPhase(sentinel::StatusLine::Phase::TEST_ORIG);
    fs::remove_all(cfg.testResultDir);
    sentinel::Subprocess firstTestProc(cfg.testCmd, cfg.timeLimit, cfg.killAfter,
                                       (ws.getOriginalDir() / "test.log").string(), cfg.silent);
    timeStamper.reset();
    firstTestProc.execute();
    double testElapsed = timeStamper.toDouble();

    if (!cfg.resuming && cfg.timeLimitStr == "auto") {
      r.baselineSecs = testElapsed;
      cfg.timeLimit = testElapsed < 1.0 ? 1 : static_cast<size_t>(ceil(testElapsed * 2.0));
      logger->info("Auto timeout: {}s (baseline {:.1f}s x2)", cfg.timeLimit, testElapsed);
    }

    if (firstTestProc.isTimedOut()) {
      logger->info("Tests timed out ({}).", timeStamper.toString(TimeStamper::Format::HumanReadable));
      logger->info("  hint: See {} for details.", (ws.getOriginalDir() / "test.log").string());
      if (!cfg.dryRun)
        throw std::runtime_error("Timeout occurs when executing test command for original source code.");
      r.testOK = false;
    }
    if (r.testOK && !validateTestResultDir(cfg.testResultDir, cfg.testResultDirStr, cfg.dryRun)) {
      logger->info("  hint: See {} for details.", (ws.getOriginalDir() / "test.log").string());
      r.testOK = false;
    }

    if (r.testOK) {
      logger->info("Tests done ({}).", timeStamper.toString(TimeStamper::Format::HumanReadable));
      sentinel::CommandRun::copyTestReportTo(cfg.testResultDir.string(),
                                             ws.getOriginalResultsDir().string(),
                                             cfg.testResultFileExts);
      ws.saveConfig(buildWorkspaceYaml(cfg.sourceRoot.string(), cfg.compileDbStr, cfg.scope,
                                       cfg.targetFileExts, cfg.diffPatterns, cfg.excludePaths,
                                       cfg.mutantLimit, cfg.buildCmd, cfg.testCmd,
                                       cfg.testResultDir.string(), cfg.testResultFileExts,
                                       cfg.coverageFiles, cfg.generatorStr, cfg.timeLimit,
                                       cfg.killAfter, cfg.randomSeed, cfg.operators,
                                       cfg.emptyOutputDir ? std::string{} : cfg.outputDir.string()));
    }
  }

  return r;
}

static PopulateResult runPopulateMutants(const RunCfg& cfg,
                                          sentinel::Workspace& ws,    // NOLINT
                                          sentinel::StatusLine& sl,   // NOLINT
                                          const BaselineResult& br,
                                          const std::shared_ptr<sentinel::Logger>& logger) {
  PopulateResult r;
  TimeStamper timeStamper;

  if (cfg.resuming) {
    r.indexedMutants = ws.loadMutants();
    logger->info("Loaded {} mutants from workspace.", r.indexedMutants.size());
    sl.setTotalMutants(r.indexedMutants.size());
    return r;
  }

  if (!cfg.dryRun || (br.buildOK && br.testOK)) {
    sl.setPhase(sentinel::StatusLine::Phase::POPULATE);
    logger->info("Generating mutants...");
    timeStamper.reset();
    auto repo = std::make_unique<sentinel::GitRepository>(cfg.sourceRoot, cfg.targetFileExts,
                                                          cfg.diffPatterns, cfg.excludePaths);
    repo->addSkipDir(cfg.workDirPath);
    sentinel::SourceLines sourceLines = repo->getSourceLines(cfg.scope);
    if (sourceLines.empty()) {
      if (cfg.scope == "commit") {
        logger->warn("No changed source lines found (scope=commit). "
                     "Make sure there are uncommitted modifications in the source tree. "
                     "Use --scope all to mutate the entire codebase.");
      } else {
        logger->warn("No source lines found to mutate. "
                     "Check --source-dir, --pattern, and --extension options.");
      }
    }
    std::shuffle(std::begin(sourceLines), std::end(sourceLines), std::mt19937(cfg.randomSeed));

    auto generator = sentinel::MutantGenerator::getInstance(cfg.generatorStr, cfg.compileDbStr);
    static const std::vector<std::string> kValidOperators = {"AOR", "BOR", "LCR", "ROR", "SDL", "SOR", "UOI"};
    for (const auto& op : cfg.operators) {
      if (std::find(kValidOperators.begin(), kValidOperators.end(), op) == kValidOperators.end()) {
        throw sentinel::InvalidArgumentException(fmt::format(
            "Invalid operator: '{}'. Valid operators: AOR, BOR, LCR, ROR, SDL, SOR, UOI", op));
      }
    }
    generator->setOperators(cfg.operators);

    sentinel::MutationFactory mutationFactory(generator);
    auto mutants = mutationFactory.populate(cfg.sourceRoot, sourceLines, cfg.mutantLimit,
                                             cfg.randomSeed, cfg.generatorStr);
    r.candidateCount = generator->getCandidateCount();

    int id = 1;
    for (auto& m : mutants) {
      logger->verbose("mutant: {}", m.str());
      ws.createMutant(id, m);
      r.indexedMutants.emplace_back(id, m);
      id++;
    }
    sl.setTotalMutants(r.indexedMutants.size());
    logger->info("Mutant population complete ({}).", timeStamper.toString(TimeStamper::Format::HumanReadable));
  }

  return r;
}

static void runMutantEvaluationLoop(const RunCfg& cfg,
                                     sentinel::Workspace& ws,    // NOLINT
                                     sentinel::StatusLine& sl,   // NOLINT
                                     sentinel::Evaluator& evaluator,  // NOLINT
                                     const std::vector<std::pair<int, sentinel::Mutant>>& indexedMutants,
                                     const std::shared_ptr<sentinel::Logger>& logger) {
  TimeStamper evalTimeStamper;
  TimeStamper mutantTimeStamper;

  fs::path actualDir = ws.getRoot() / "actual";
  auto repo = std::make_unique<sentinel::GitRepository>(cfg.sourceRoot, cfg.targetFileExts,
                                                        cfg.diffPatterns, cfg.excludePaths);
  repo->addSkipDir(cfg.workDirPath);
  sentinel::CoverageInfo cov(cfg.coverageFiles);

  sl.setPhase(sentinel::StatusLine::Phase::MUTANT);

  size_t totalMutants = indexedMutants.size();
  logger->info("Evaluating {} mutant{}...", totalMutants, totalMutants == 1 ? "" : "s");
  evalTimeStamper.reset();

  size_t localIdx = 0;
  for (const auto& [id, m] : indexedMutants) {
    ++localIdx;
    if (ws.isDone(id)) {
      sentinel::MutationResult prevResult = ws.getDoneResult(id);
      evaluator.injectResult(prevResult);
      logger->verbose("Mutant #{} already done: {}. Skipping.", id,
                      sentinel::MutationStateToStr(prevResult.getMutationState()));
      sl.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                       m.getFirst().line);
      sl.recordResult(static_cast<int>(prevResult.getMutationState()));
      continue;
    }

    if (ws.isLocked(id)) {
      logger->info("Mutant #{} was interrupted mid-run. Restoring source...", id);
      sentinel::CommandRun::restoreBackup(ws.getBackupDir().string(), cfg.sourceRoot.string());
      ws.clearLock(id);
    }

    if (!cfg.coverageFiles.empty() && !cov.cover(m.getPath().string(), m.getFirst().line)) {
      sentinel::MutationResult result = evaluator.compare(m, actualDir.string(), "uncovered");
      ws.setDone(id, result);
      sl.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                       m.getFirst().line);
      sl.recordResult(static_cast<int>(result.getMutationState()));
      logger->info("[{}/{}] {} @ {}:{} → {}",
                   localIdx, totalMutants, m.getOperator(),
                   m.getPath().filename().string(), m.getFirst().line,
                   sentinel::MutationStateToStr(result.getMutationState()));
      continue;
    }

    ws.setLock(id);
    sl.setMutantInfo(static_cast<size_t>(id), m.getOperator(), m.getPath().filename().string(),
                     m.getFirst().line);
    mutantTimeStamper.reset();

    std::stringstream buf;
    buf << m;
    logger->verbose("Applying: {}", buf.str());
    repo->getSourceTree()->modify(m, ws.getBackupDir().string());

    logger->verbose("Building mutant #{}...", id);
    sentinel::Subprocess buildProc(cfg.buildCmd, 0, 0,
                                   (ws.getMutantDir(id) / "build.log").string(), cfg.silent);
    buildProc.execute();
    std::string testState = "success";
    bool buildSuccess = buildProc.isSuccessfulExit();

    if (buildSuccess) {
      logger->verbose("Build OK, running tests...");
      fs::remove_all(cfg.testResultDir);
      sentinel::Subprocess proc(cfg.testCmd, cfg.timeLimit, cfg.killAfter,
                                (ws.getMutantDir(id) / "test.log").string(), cfg.silent);
      proc.execute();
      if (proc.isTimedOut()) {
        testState = "timeout";
        logger->warn("Test timed out.");
        fs::remove_all(actualDir);
        fs::remove_all(cfg.testResultDir);
      } else {
        sentinel::CommandRun::copyTestReportTo(cfg.testResultDir.string(), actualDir.string(),
                                               cfg.testResultFileExts);
        fs::remove_all(cfg.testResultDir);
      }
    } else {
      testState = "build_failure";
      logger->verbose("Build failed.");
    }

    sentinel::MutationResult result = evaluator.compare(m, actualDir.string(), testState);

    logger->verbose("Restoring source...");
    sentinel::CommandRun::restoreBackup(ws.getBackupDir().string(), cfg.sourceRoot.string());
    ws.clearLock(id);
    ws.setDone(id, result);
    sl.recordResult(static_cast<int>(result.getMutationState()));
    logger->info("[{}/{}] {} @ {}:{} → {}  ({})",
                 localIdx, totalMutants, m.getOperator(),
                 m.getPath().filename().string(), m.getFirst().line,
                 sentinel::MutationStateToStr(result.getMutationState()),
                 mutantTimeStamper.toString(TimeStamper::Format::HumanReadable));
    fs::remove_all(actualDir);
  }

  logger->info("Evaluation done ({}).", evalTimeStamper.toString(TimeStamper::Format::HumanReadable));
}

static int generateReportAndScore(const RunCfg& cfg,
                                  const sentinel::Evaluator& evaluator,
                                  sentinel::StatusLine* sl,
                                  const std::shared_ptr<sentinel::Logger>& logger) {
  sl->setPhase(sentinel::StatusLine::Phase::REPORT);

  fs::path outputDir = cfg.outputDir;
  fs::create_directories(outputDir);
  outputDir = fs::canonical(outputDir);

  if (!cfg.emptyOutputDir) {
    logger->info("Writing report to {}...", outputDir.string());
    sentinel::XMLReport xmlReport(evaluator.getMutationResults(), cfg.sourceRoot);
    xmlReport.save(outputDir);
    sentinel::HTMLReport htmlReport(evaluator.getMutationResults(), cfg.sourceRoot);
    htmlReport.save(outputDir);
    htmlReport.printSummary();
  } else {
    sentinel::XMLReport xmlReport(evaluator.getMutationResults(), cfg.sourceRoot);
    xmlReport.printSummary();
  }

  size_t totalMutants = 0;
  size_t killedMutants = 0;
  for (const auto& r : evaluator.getMutationResults()) {
    auto s = r.getMutationState();
    if (s == sentinel::MutationState::BUILD_FAILURE ||
        s == sentinel::MutationState::RUNTIME_ERROR ||
        s == sentinel::MutationState::TIMEOUT) {
      continue;
    }
    totalMutants++;
    if (r.getDetected()) killedMutants++;
  }
  if (totalMutants > 0) {
    double score = (100.0 * killedMutants) / totalMutants;
    Console::err("[info] Mutation score: {:.1f}% ({}/{} mutants killed)",
                 score, killedMutants, totalMutants);
    if (cfg.threshold && score < *cfg.threshold) {
      Console::err("[error] Mutation score {:.1f}% is below threshold {:.1f}%",
                   score, *cfg.threshold);
      return 3;
    }
  } else {
    Console::err("[info] Mutation score: -% (no evaluable mutants)");
  }
  return 0;
}

int CommandRun::run() {
  if (mInit) {
    static const char* const kConfigFileName = "sentinel.yaml";
    if (fs::exists(kConfigFileName)) {
      bool overwrite = mForce.Get();
      if (!overwrite) {
        overwrite = Console::confirm("'{}' already exists. Overwrite?", kConfigFileName);
      }
      if (!overwrite) {
        Console::out("Aborted.");
        return 0;
      }
    }
    std::ofstream out(kConfigFileName);
    if (!out) {
      throw std::runtime_error(fmt::format("Failed to create '{}'", kConfigFileName));
    }
    out << kYamlTemplate;
    Console::out("Generated '{}'", kConfigFileName);
    return 0;
  }

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
    bool resume = !mForce.Get() && !dryRun;
    if (resume) {
      resume = Console::confirm("Previous run found in '{}'. Resume?", workDirPath.string());
    }
    if (resume) {
      resuming = true;
      activeConfig = SentinelConfig::loadFromFile((ws.getRoot() / "sentinel.yaml").string());
      Console::out("Resuming previous run.");
    } else {
      ws.initialize();
      if (!dryRun) {
        Console::out("Starting fresh run.");
      }
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
  bool silent = mSilent.Get();

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

  // Validate threshold
  {
    std::optional<double> thresh = getThreshold();
    if (thresh && (*thresh < 0.0 || *thresh > 100.0)) {
      throw InvalidArgumentException(
          fmt::format("Invalid --threshold value: {:.1f}. Expected a percentage in [0, 100].", *thresh));
    }
  }

  // Validate partition
  auto [partIdx, partCount] = getPartition();
  if (partIdx != 0 && !resuming) {
    bool hasSeed = (mSeed && mSeed.Get() != "auto") || (mConfig && mConfig->seed.has_value());
    if (!hasSeed) {
      throw InvalidArgumentException(
          "--partition requires an explicit --seed value.\n"
          "  Without a fixed seed, each partition instance generates a different random\n"
          "  mutant list, making combined results inconsistent with a single full run.\n"
          "  hint: add --seed <N> with the same value across all partition instances,\n"
          "        e.g., --seed 42 --partition 1/5");
    }
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
  workspaceDirForSH = ws.getRoot();
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
          deleteDir = Console::confirm("The given test result path is not empty: {0}\n"
                                       "Delete and continue?", testResultDirStr);
        }
        if (!deleteDir) {
          throw InvalidArgumentException("Aborted.");
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
    if (!checkConfigWarnings(sourceRoot, mutantLimit, timeLimitStr, diffPatterns, excludePaths,
                             partIdx, partCount)) {
      return 0;
    }
  }

  auto logger = Logger::getLogger(cCommandRunLoggerName);

  if (resuming) {
    logger->warn("Resuming with saved config: {}. "
                 "New CLI options and sentinel.yaml changes are ignored.",
                 (ws.getRoot() / "sentinel.yaml").string());
  }

  logger->verbose("source root:        {}", sourceRoot.string());
  logger->verbose("build cmd:          {}", buildCmd);
  logger->verbose("compiledb dir:      {}", compileDbStr);
  logger->verbose("test cmd:           {}", testCmd);
  logger->verbose("test result dir:    {}", testResultDir.string());
  logger->verbose("test result exts:   {}", sentinel::string::join(", ", testResultFileExts));
  logger->verbose("timeout:            {}s", mTimeLimit);
  logger->verbose("kill-after:         {}s", mKillAfter);
  logger->verbose("scope:              {}", scope);
  logger->verbose("source extensions:  {}", sentinel::string::join(", ", targetFileExts));
  logger->verbose("patterns:           {}", sentinel::string::join(", ", diffPatterns));
  logger->verbose("exclude patterns:   {}", sentinel::string::join(", ", excludePaths));
  logger->verbose("mutant limit:       {}", mutantLimit);
  logger->verbose("generator:          {}", generatorStr);
  logger->verbose("random seed:        {}", randomSeed);
  logger->verbose("coverage files:     {}", sentinel::string::join(", ", coverageFiles));
  logger->info("Workspace: {}", ws.getRoot().string());
  logger->verbose("resuming:           {}", resuming ? "yes" : "no");

  bool disableStatusLine = static_cast<bool>(mNoStatusLine);
  if (!disableStatusLine) {
    statusLine.enable();
  }

  for (const auto& filename : coverageFiles) {
    if (!fs::exists(filename)) {
      throw InvalidArgumentException(fmt::format("Input coverage file does not exist: {}", filename));
    }
  }

  // Build resolved configuration bundle for the phase helpers.
  RunCfg cfg;
  cfg.sourceRoot = sourceRoot;
  cfg.workDirPath = workDirPath;
  cfg.buildCmd = buildCmd;
  cfg.testCmd = testCmd;
  cfg.testResultDir = testResultDir;
  cfg.testResultDirStr = testResultDirStr;
  cfg.testResultFileExts = testResultFileExts;
  cfg.coverageFiles = coverageFiles;
  cfg.scope = scope;
  cfg.generatorStr = generatorStr;
  cfg.mutantLimit = mutantLimit;
  cfg.timeLimit = mTimeLimit;
  cfg.killAfter = mKillAfter;
  cfg.randomSeed = randomSeed;
  cfg.targetFileExts = targetFileExts;
  cfg.excludePaths = excludePaths;
  cfg.diffPatterns = diffPatterns;
  cfg.operators = operators;
  cfg.outputDir = outputDir;
  cfg.emptyOutputDir = emptyOutputDir;
  cfg.silent = silent;
  cfg.compileDbStr = compileDbStr;
  cfg.timeLimitStr = timeLimitStr;
  cfg.resuming = resuming;
  cfg.dryRun = dryRun;
  cfg.disableStatusLine = disableStatusLine;
  cfg.threshold = getThreshold();

  int exitCode = 0;

  try {
    // ── STEP 3: Original build & test ──────────────────────────────────────────
    BaselineResult br = runBaselineBuildAndTest(cfg, ws, statusLine, logger);

    // Restore any leftover backup from a previously interrupted run.
    restoreBackup(ws.getBackupDir().string(), sourceRoot.string());

    // ── STEP 4: Populate or reload mutants ─────────────────────────────────────
    PopulateResult pr = runPopulateMutants(cfg, ws, statusLine, br, logger);

    // ── STEP 4b: Apply partition slice ──────────────────────────────────────────
    size_t fullMutantCount = pr.indexedMutants.size();
    if (partIdx != 0) {
      if (pr.indexedMutants.empty()) {
        logger->warn("Partition {}/{}: no mutants were generated (total: 0). "
                     "Nothing to evaluate.",
                     partIdx, partCount);
      } else {
        size_t start = (partIdx - 1) * fullMutantCount / partCount;
        size_t end = partIdx * fullMutantCount / partCount;
        pr.indexedMutants = std::vector<std::pair<int, sentinel::Mutant>>(
            pr.indexedMutants.begin() + static_cast<ptrdiff_t>(start),
            pr.indexedMutants.begin() + static_cast<ptrdiff_t>(end));
        statusLine.setTotalMutants(pr.indexedMutants.size());
        if (pr.indexedMutants.empty()) {
          logger->warn("Partition {}/{}: 0 mutants assigned out of {} total. "
                       "Increase --limit or reduce --partition TOTAL.",
                       partIdx, partCount, fullMutantCount);
        } else {
          logger->info("Partition {}/{}: evaluating {} of {} mutants.",
                       partIdx, partCount, pr.indexedMutants.size(), fullMutantCount);
        }
      }
    }

    // ── DRY-RUN EXIT ────────────────────────────────────────────────────────────
    if (dryRun) {
      statusLine.disable();
      gStatusLineForSH = nullptr;
      printDryRunSummary(sourceRoot, buildCmd, testCmd, scope, mutantLimit,
                         br.baselineSecs, cfg.timeLimit, timeLimitStr,
                         pr.indexedMutants, pr.candidateCount, getVerbose(),
                         br.buildOK, br.testOK, workDirPath.string(), operators,
                         partIdx, partCount, fullMutantCount);
      std::raise(SIGUSR1);
      return (br.buildOK && br.testOK) ? 0 : 1;
    }

    // ── STEP 5: Mutant evaluation loop ─────────────────────────────────────────
    Evaluator evaluator(ws.getOriginalResultsDir().string(), sourceRoot.string());
    runMutantEvaluationLoop(cfg, ws, statusLine, evaluator, pr.indexedMutants, logger);

    // ── STEP 6+7: Report & score threshold check ───────────────────────────────
    exitCode = generateReportAndScore(cfg, evaluator, &statusLine, logger);
  } catch (...) {
    std::raise(SIGUSR1);
    throw;
  }

  statusLine.disable();
  gStatusLineForSH = nullptr;
  std::raise(SIGUSR1);
  return exitCode;
}

void CommandRun::copyTestReportTo(const std::string& from, const std::string& to,
                                  const std::vector<std::string>& exts) {
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

std::optional<double> CommandRun::getThreshold() {
  if (mThreshold) {
    return mThreshold.Get();
  }
  if (mConfig && mConfig->threshold) {
    return *mConfig->threshold;
  }
  return std::nullopt;
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

bool CommandRun::checkConfigWarnings(
    const std::filesystem::path& sourceRoot,
    size_t mutantLimit,
    const std::string& timeLimitStr,
    const std::vector<std::string>& diffPatterns,
    const std::vector<std::string>& excludePaths,
    size_t partIdx,
    size_t partCount) {
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

  // limit > 0 but smaller than partition count: most partitions receive 0 mutants.
  if (mutantLimit > 0 && partIdx != 0 && mutantLimit < partCount) {
    warnings.push_back(fmt::format(
        "{}: {} is smaller than --partition TOTAL ({}). "
        "Most partitions will receive 0 mutants due to integer division. "
        "Use --limit=0 (unlimited) or set --limit >= {}.",
        limitSrc, mutantLimit, partCount, partCount));
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
    Console::out("\nConfiguration warnings:");
    for (const auto& w : warnings) {
      Console::out("  [!] {}", w);
    }
    if (!Console::confirm("\nProceed?")) {
      Console::out("Aborted.");
      return false;
    }
  }
  return true;
}

std::pair<size_t, size_t> CommandRun::getPartition() {
  if (!mPartition) {
    return {0, 0};
  }
  const std::string& s = mPartition.Get();
  auto slash = s.find('/');
  if (slash == std::string::npos || slash == 0 || slash + 1 == s.size()) {
    throw InvalidArgumentException(
        fmt::format("Invalid --partition value: '{}'. "
                    "Expected format: N/TOTAL (e.g., --partition=2/5).", s));
  }
  size_t idx = 0;
  size_t cnt = 0;
  try {
    idx = std::stoul(s.substr(0, slash));
    cnt = std::stoul(s.substr(slash + 1));
  } catch (...) {
    throw InvalidArgumentException(
        fmt::format("Invalid --partition value: '{}'. "
                    "N and TOTAL must be positive integers (e.g., --partition=2/5).", s));
  }
  if (cnt == 0) {
    throw InvalidArgumentException(
        fmt::format("Invalid --partition value: '{}'. TOTAL must be at least 1.", s));
  }
  if (idx == 0 || idx > cnt) {
    throw InvalidArgumentException(
        fmt::format("Invalid --partition value: '{}'. "
                    "N must be between 1 and TOTAL (got N={}, TOTAL={}).", s, idx, cnt));
  }
  return {idx, cnt};
}

}  // namespace sentinel
