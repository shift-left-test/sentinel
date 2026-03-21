/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/wait.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>
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
#include "sentinel/Config.hpp"
#include "sentinel/Console.hpp"
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/MutationRunner.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/YamlConfigParser.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/signal.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static const char* cMutationRunnerLoggerName = "MutationRunner";
static fs::path backupDirForSH;
static fs::path sourceRootForSH;
static fs::path workspaceDirForSH;
static StatusLine* gStatusLineForSH = nullptr;

static void signalHandler(int signum) {
  if (!backupDirForSH.empty() && fs::is_directory(backupDirForSH)) {
    MutationRunner::restoreBackup(backupDirForSH, sourceRootForSH);
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

static void printDryRunSummary(const Config& cfg, size_t computedTimeLimit,
                               const std::vector<std::pair<int, Mutant>>& indexedMutants,
                               std::size_t candidateCount, const std::filesystem::path& workspaceDir,
                               size_t partIdx, size_t partCount) {
  Console::out("\n=== Sentinel Dry Run ===");
  Console::out("  source-dir:    {}", cfg.sourceDir->string());
  Console::out("  build-command: {}", cfg.buildCmd.value_or(""));
  Console::out("  test-command:  {}", cfg.testCmd.value_or(""));
  Console::out("  scope:         {}", cfg.scope.value_or("all"));
  Console::out("  limit:         {}", cfg.limit.value_or(0) == 0 ? "unlimited" : std::to_string(*cfg.limit));
  Console::out("  operators:     {}", cfg.operators->empty() ? "all" : sentinel::string::join(", ", *cfg.operators));
  if (partIdx != 0) {
    Console::out("  partition:     {}/{}", partIdx, partCount);
  }
  Console::out("  workspace:     {}", workspaceDir.string());

  Console::out("\n  {}  Original build", "[ OK ]");  // Baseline run already happened

  if (cfg.timeLimit == "auto") {
    Console::out("  {}  Original tests  (auto-timeout: {}s)", "[ OK ]", computedTimeLimit);
  } else {
    Console::out("  {}  Original tests  (timeout: {}s)", "[ OK ]", computedTimeLimit);
  }

  if (indexedMutants.empty()) {
    Console::out("  [WARN]  Mutants: 0 — nothing to evaluate.");
  } else {
    Console::out("  [ OK ]  Mutants: {} of {} candidates", indexedMutants.size(), candidateCount);
  }

  if (cfg.verbose && *cfg.verbose) {
    for (const auto& [id, m] : indexedMutants) {
      Console::out("          [{:3d}] {} @ {}:{}", id, m.getOperator(),
                   m.getPath().filename().string(), m.getFirst().line);
    }
  }

  if (!indexedMutants.empty()) {
    Console::out("\nWorkspace saved. Remove --dry-run to start mutation testing");
  }
}

MutationRunner::MutationRunner(const Config& config) : mConfig(config) {}

void MutationRunner::init() {
  auto logger = Logger::getLogger("sentinel");
  if (mConfig.debug && *mConfig.debug) {
    logger->setLevel(Logger::Level::DEBUG);
  } else if (mConfig.verbose && *mConfig.verbose) {
    logger->setLevel(Logger::Level::VERBOSE);
  }
}

void MutationRunner::setSignalHandler() {
  signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1},
                                    signalHandler);
}

static std::string buildWorkspaceYaml(const Config& cfg, size_t computedTimeout) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "version" << YAML::Value << 1;
  out << YAML::Key << "source-dir" << YAML::Value << cfg.sourceDir->string();
  if (cfg.outputDir && !cfg.outputDir->empty()) {
    out << YAML::Key << "output-dir" << YAML::Value << cfg.outputDir->string();
  }
  out << YAML::Key << "compiledb-dir" << YAML::Value << cfg.compileDbDir->string();
  out << YAML::Key << "scope" << YAML::Value << cfg.scope.value_or("all");
  out << YAML::Key << "extension" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : *cfg.extensions) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "pattern" << YAML::Value << YAML::BeginSeq;
  for (const auto& p : *cfg.patterns) out << p;
  out << YAML::EndSeq;
  out << YAML::Key << "exclude" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : *cfg.excludes) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "limit" << YAML::Value << cfg.limit.value_or(0);
  out << YAML::Key << "build-command" << YAML::Value << cfg.buildCmd.value_or("");
  out << YAML::Key << "test-command" << YAML::Value << cfg.testCmd.value_or("");
  out << YAML::Key << "test-report-dir" << YAML::Value << cfg.testResultDir->string();
  out << YAML::Key << "test-report-extension" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : *cfg.testResultFileExts) out << e;
  out << YAML::EndSeq;
  out << YAML::Key << "coverage" << YAML::Value << YAML::BeginSeq;
  for (const auto& c : *cfg.coverageFiles) out << c.string();
  out << YAML::EndSeq;
  out << YAML::Key << "generator" << YAML::Value << cfg.generator.value_or("uniform");
  out << YAML::Key << "timeout" << YAML::Value << computedTimeout;
  out << YAML::Key << "kill-after" << YAML::Value << cfg.killAfter.value_or("60");
  if (cfg.seed) out << YAML::Key << "seed" << YAML::Value << *cfg.seed;
  out << YAML::Key << "operator" << YAML::Value << YAML::BeginSeq;
  for (const auto& op : *cfg.operators) out << op;
  out << YAML::EndSeq;
  out << YAML::EndMap;
  return out.c_str();
}

static const char* const kYamlTemplate =
    "# sentinel.yaml - full configuration template\n"
    "#\n"
    "# Uncomment and edit the options you need.\n"
    "# CLI arguments always take priority over values in this file.\n"
    "\n"
    "# Config file format version (required)\n"
    "version: 1\n"
    "\n"
    "# Directory for output reports (default: none)\n"
    "# output-dir: ./sentinel_output\n"
    "\n"
    "# Workspace directory for all sentinel run artifacts (default: ./sentinel_workspace)\n"
    "# workspace: ./sentinel_workspace\n"
    "\n"
    "# --- Run options ---\n"
    "\n"
    "# Fail with exit code 3 if mutation score is below this percentage 0-100 (default: disabled)\n"
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
    "# Test time limit in seconds (default: auto - 2x baseline run time; 0 = no limit)\n"
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
    "# Paths or glob patterns to constrain the diff (default: none - entire source)\n"
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
    "# Random seed for mutant selection (default: auto - picked randomly)\n"
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

int MutationRunner::run() {
  if (mConfig.init) {
    static const char* const kConfigFileName = "sentinel.yaml";
    if (fs::exists(kConfigFileName)) {
      bool overwrite = mConfig.force && *mConfig.force;
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

  bool dryRun = mConfig.dryRun;
  StatusLine statusLine;
  statusLine.setDryRun(dryRun);
  if (!mConfig.noStatusLine) statusLine.enable();
  gStatusLineForSH = &statusLine;

  fs::path workDirPath = fs::absolute(*mConfig.workDir);
  Workspace ws(workDirPath);
  bool resuming = ws.hasPreviousRun() && !mConfig.force && !dryRun &&
                  Console::confirm("Previous run found in '{}'. Resume?", workDirPath.string());

  Config activeConfig = mConfig;
  if (resuming) {
    activeConfig = YamlConfigParser::loadFromFile(ws.getRoot() / "sentinel.yaml");
    Console::out("Resuming previous run.");
  } else {
    ws.initialize();
  }

  // Pre-run configuration validation
  if (!resuming && !checkConfigWarnings()) {
    return 0;
  }

  // Validate required options
  if (activeConfig.buildCmd->empty()) {
    throw InvalidArgumentException("Option --build-command is required.");
  }
  if (activeConfig.testCmd->empty()) {
    throw InvalidArgumentException("Option --test-command is required.");
  }
  if (activeConfig.testResultDir->empty()) {
    throw InvalidArgumentException("Option --test-report-dir is required.");
  }

  // Validate threshold
  if (activeConfig.threshold && (*activeConfig.threshold < 0.0 || *activeConfig.threshold > 100.0)) {
    throw InvalidArgumentException(
        fmt::format("Invalid --threshold value: {:.1f}. Expected a percentage in [0, 100].", *activeConfig.threshold));
  }

  // Validate partition
  size_t partIdx = 0;
  size_t partCount = 0;
  if (activeConfig.partition && !activeConfig.partition->empty()) {
    const std::string& s = *activeConfig.partition;
    auto slash = s.find('/');
    if (slash == std::string::npos || slash == 0 || slash + 1 == s.size()) {
      throw InvalidArgumentException(fmt::format("Invalid --partition value: '{}'. Expected format: N/TOTAL.", s));
    }
    try {
      partIdx = std::stoul(s.substr(0, slash));
      partCount = std::stoul(s.substr(slash + 1));
    } catch (...) {
      throw InvalidArgumentException(
          fmt::format("Invalid --partition value: '{}'. N and TOTAL must be positive integers.", s));
    }
    if (partCount == 0 || partIdx == 0 || partIdx > partCount) {
      throw InvalidArgumentException(fmt::format("Invalid --partition value: '{}'. N must be between 1 and TOTAL.", s));
    }
    if (!activeConfig.seed) {
      throw InvalidArgumentException("--partition requires an explicit --seed value.");
    }
  }

  auto logger = Logger::getLogger(cMutationRunnerLoggerName);
  sourceRootForSH = *activeConfig.sourceDir;
  backupDirForSH = ws.getBackupDir();
  workspaceDirForSH = ws.getRoot();
  setSignalHandler();

  // Baseline & Mutants
  size_t computedTimeLimit = 0;
  if (activeConfig.timeLimit != "auto") {
    computedTimeLimit = std::stoul(*activeConfig.timeLimit);
  }

  // Original Build & Test (Simplification: inlining logic for readability)
  if (!resuming) {
    statusLine.setPhase(StatusLine::Phase::BUILD_ORIG);
    auto buildLog = ws.getOriginalDir() / "build.log";
    Subprocess buildProc(*activeConfig.buildCmd, 0, 0, buildLog.string(), *activeConfig.silent);
    buildProc.execute();
    if (!buildProc.isSuccessfulExit()) {
      throw std::runtime_error(fmt::format("Baseline build failed. See: {}", buildLog.string()));
    }
    if (!fs::exists(*activeConfig.compileDbDir / "compile_commands.json")) {
      throw std::runtime_error(fmt::format(
          "compile_commands.json not found in '{}'. "
          "Make sure build-command generates it (e.g., add -DCMAKE_EXPORT_COMPILE_COMMANDS=ON to cmake).",
          activeConfig.compileDbDir->string()));
    }

    statusLine.setPhase(StatusLine::Phase::TEST_ORIG);
    Timestamper ts;
    auto testLog = ws.getOriginalDir() / "test.log";
    Subprocess testProc(*activeConfig.testCmd, computedTimeLimit, std::stoul(*activeConfig.killAfter),
                        testLog.string(), *activeConfig.silent);
    ts.reset();
    testProc.execute();
    if (activeConfig.timeLimit == "auto") {
      computedTimeLimit = static_cast<size_t>(ceil(ts.toDouble() * 2.0));
      if (computedTimeLimit < 1) computedTimeLimit = 1;
    }
    copyTestReportTo(*activeConfig.testResultDir, ws.getOriginalResultsDir(),
                     *activeConfig.testResultFileExts);
    if (fs::is_empty(ws.getOriginalResultsDir())) {
      throw std::runtime_error(fmt::format(
          "No test result files found in '{}' after running test command. See: {}",
          activeConfig.testResultDir->string(), testLog.string()));
    }
    ws.saveConfig(buildWorkspaceYaml(activeConfig, computedTimeLimit));
  }

  // Populate
  std::vector<std::pair<int, Mutant>> indexedMutants;
  if (resuming) {
    indexedMutants = ws.loadMutants();
  } else {
    statusLine.setPhase(StatusLine::Phase::POPULATE);
    auto repo = std::make_unique<GitRepository>(*activeConfig.sourceDir, *activeConfig.extensions,
                                                *activeConfig.patterns, *activeConfig.excludes);
    repo->addSkipDir(workDirPath);
    SourceLines sourceLines = repo->getSourceLines(*activeConfig.scope);
    unsigned int seed = activeConfig.seed ? *activeConfig.seed : std::random_device {}();
    std::shuffle(sourceLines.begin(), sourceLines.end(), std::mt19937(seed));

    auto generator = MutantGenerator::getInstance(*activeConfig.generator, *activeConfig.compileDbDir);
    generator->setOperators(*activeConfig.operators);
    MutationFactory factory(generator);
    auto mutants = factory.populate(*activeConfig.sourceDir, sourceLines, *activeConfig.limit, seed,
                                    *activeConfig.generator);
    if (mutants.size() > static_cast<std::size_t>(Workspace::kMaxMutantCount)) {
      throw std::runtime_error(fmt::format(
          "Too many mutants: {} generated, maximum is {}. "
          "Use --limit to reduce the number of mutants.",
          mutants.size(), Workspace::kMaxMutantCount));
    }
    int id = 1;
    for (auto& m : mutants) {
      ws.createMutant(id, m);
      indexedMutants.emplace_back(id, m);
      id++;
    }
  }
  statusLine.setTotalMutants(indexedMutants.size());

  if (dryRun) {
    statusLine.disable();
    printDryRunSummary(activeConfig, computedTimeLimit, indexedMutants, indexedMutants.size(),
                       workDirPath, partIdx, partCount);
    return 0;
  }

  // Evaluation Loop
  Evaluator evaluator(ws.getOriginalResultsDir(), *activeConfig.sourceDir);
  statusLine.setPhase(StatusLine::Phase::MUTANT);
  const fs::path backupDir = ws.getBackupDir();
  const fs::path actualDir = ws.getRoot() / "actual";
  const unsigned long killAfterSecs = std::stoul(*activeConfig.killAfter);
  for (const auto& [id, m] : indexedMutants) {
    if (ws.isDone(id)) {
      auto doneResult = ws.getDoneResult(id);
      evaluator.injectResult(doneResult);
      statusLine.recordResult(static_cast<int>(doneResult.getMutationState()));
      continue;
    }
    ws.setLock(id);
    statusLine.setMutantInfo(id, m.getOperator(), m.getPath().filename().string(), m.getFirst().line);

    // Apply mutation
    auto repo = std::make_unique<GitRepository>(*activeConfig.sourceDir, *activeConfig.extensions);
    repo->getSourceTree()->modify(m, backupDir.string());

    const fs::path mutantDir = ws.getMutantDir(id);
    Subprocess mBuild(*activeConfig.buildCmd, 0, 0, (mutantDir / "build.log").string(), *activeConfig.silent);
    mBuild.execute();

    std::string testState = "success";
    if (mBuild.isSuccessfulExit()) {
      fs::remove_all(*activeConfig.testResultDir);
      Subprocess mTest(*activeConfig.testCmd, computedTimeLimit, killAfterSecs,
                        (mutantDir / "test.log").string(), *activeConfig.silent);
      mTest.execute();
      if (mTest.isTimedOut()) {
        testState = "timeout";
      } else {
        copyTestReportTo(*activeConfig.testResultDir, actualDir, *activeConfig.testResultFileExts);
      }
    } else {
      testState = "build_failure";
    }

    MutationResult result = evaluator.compare(m, actualDir, testState);
    restoreBackup(backupDir, *activeConfig.sourceDir);
    ws.clearLock(id);
    ws.setDone(id, result);
    statusLine.recordResult(static_cast<int>(result.getMutationState()));
    fs::remove_all(actualDir);
  }

  // Report
  statusLine.setPhase(StatusLine::Phase::REPORT);
  XMLReport xmlReport(evaluator.getMutationResults(), *activeConfig.sourceDir);
  xmlReport.printSummary();

  if (activeConfig.outputDir && !activeConfig.outputDir->empty()) {
    xmlReport.save(*activeConfig.outputDir);
    HTMLReport htmlReport(evaluator.getMutationResults(), *activeConfig.sourceDir);
    htmlReport.save(*activeConfig.outputDir);
  }

  statusLine.disable();
  gStatusLineForSH = nullptr;
  return 0;
}

void MutationRunner::copyTestReportTo(const std::filesystem::path& from, const std::filesystem::path& to,
                                      const std::vector<std::string>& exts) {
  fs::remove_all(to);
  fs::create_directories(to);
  if (fs::is_directory(from)) {
    for (const auto& dirent : fs::recursive_directory_iterator(from)) {
      if (dirent.is_regular_file()) {
        std::string ext = dirent.path().extension().string();
        if (ext.size() > 1) ext = ext.substr(1);
        if (exts.empty() || std::find(exts.begin(), exts.end(), ext) != exts.end()) {
          fs::copy(dirent.path(), to);
        }
      }
    }
  }
}

void MutationRunner::restoreBackup(const std::filesystem::path& backup, const std::filesystem::path& srcRoot) {
  for (const auto& dirent : fs::directory_iterator(backup)) {
    fs::copy(dirent.path(), srcRoot / dirent.path().filename(),
             fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    fs::remove_all(dirent.path());
  }
}

bool MutationRunner::checkConfigWarnings() {
  std::vector<std::string> warnings;

  // limit=0: evaluates every candidate mutant, which may take hours.
  if (mConfig.limit && *mConfig.limit == 0) {
    warnings.push_back("limit: 0 - all candidate mutants will be evaluated. This may take a very long time.");
  }

  // timeout=0: no per-mutant time cap; a hanging test blocks the run forever.
  if (mConfig.timeLimit && *mConfig.timeLimit == "0") {
    warnings.push_back("timeout: 0 - no per-mutant test time limit. A hanging test will block the run indefinitely.");
  }

  // --exclude checks
  if (mConfig.excludes) {
    for (const auto& excl : *mConfig.excludes) {
      if (!excl.empty() && excl.back() == '/') {
        warnings.push_back(fmt::format("exclude: '{}' ends with '/'. "
                                       "Patterns are matched against file paths, not directories.", excl));
      } else if (!excl.empty() && excl.front() != '*' && !fs::path(excl).is_absolute()) {
        warnings.push_back(fmt::format("exclude: '{}' is a relative pattern without a leading '*'.", excl));
      }
    }
  }

  // --pattern checks
  if (mConfig.patterns) {
    fs::path srcRoot = *mConfig.sourceDir;

    for (const auto& pat : *mConfig.patterns) {
      fs::path patPath(pat);
      if (patPath.is_absolute()) {
        // sourceDir is canonical absolute
        auto rel = patPath.lexically_relative(srcRoot);
        if (rel.empty() || rel.native().find("..") != std::string::npos) {
          warnings.push_back(fmt::format("pattern: '{}' is an absolute path outside source-dir.", pat));
        } else {
           warnings.push_back(fmt::format("pattern: '{}' is an absolute path. "
                                         "Git pathspec uses paths relative to repository root.", pat));
        }
      }
    }
  }

  if (!warnings.empty() && !(mConfig.force && *mConfig.force)) {
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

}  // namespace sentinel
