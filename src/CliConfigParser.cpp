/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include <vector>
#include "sentinel/CliConfigParser.hpp"

namespace sentinel {

CliConfigParser::CliConfigParser(args::ArgumentParser& parser) :
    mGroupRunCtrl(parser, "Run options:"),
    mGroupSetup(parser, "Setup options:"),
    mGroupBuildTest(parser, "Build & test options:"),
    mGroupMutation(parser, "Mutation options:"),
    mGroupAdvanced(parser, "Advanced options:"),
    mConfigFile(mGroupRunCtrl, "PATH", "Path to a YAML config file (default: sentinel.yaml in current directory)",
                {"config"}),
    mWorkDir(mGroupRunCtrl, "PATH", "Workspace directory for all sentinel run artifacts.", {"workspace"}),
    mClean(mGroupRunCtrl, "clean", "Clear workspace and start a fresh run.", {'c', "clean"}),
    mOutputDir(mGroupRunCtrl, "PATH", "Directory for output reports.", {'o', "output-dir"}),
    mDryRun(mGroupRunCtrl, "dry-run",
            "Build, test, and generate mutants then exit; prints a readiness summary without evaluating mutants",
            {'n', "dry-run"}),
    mVerbose(mGroupRunCtrl, "verbose", "Show build/test subprocess output and enable verbose logging.",
             {'v', "verbose"}),
    mNoTTY(mGroupRunCtrl, "no-tty",
           "Disable TTY features: suppress the live status line and use text-based progress logging instead",
           {"no-tty"}),
    mInit(mGroupSetup, "init", "Write a sentinel.yaml config template to the current directory and exit", {"init"}),
    mForce(mGroupSetup, "force", "Overwrite existing files (used with --init).", {"force"}),
    mSourceDir(mGroupBuildTest, "PATH", "Path to the root of the source tree.", {"source-dir"}),
    mBuildCmd(mGroupBuildTest, "CMD", "Shell command to build the project", {"build-command"}),
    mCompileDbDir(mGroupBuildTest, "PATH", "Path to the directory containing compile_commands.json.",
                  {"compiledb-dir"}),
    mTestCmd(mGroupBuildTest, "CMD", "Shell command to run tests", {"test-command"}),
    mTestResultDir(mGroupBuildTest, "PATH", "Path to the test report directory", {"test-result-dir"}),
    mTestResultExts(mGroupBuildTest, "EXT", "File extension of the test report", {"test-result-ext"}),
    mTimeout(mGroupBuildTest, "SEC", "Test time limit in seconds; 0 = no limit (default: 1.5x original test time)",
             {"timeout"}),
    mKillAfter(mGroupBuildTest, "SEC", "Seconds to wait after timeout before sending SIGKILL (0 = disabled)",
               {"kill-after"}),
    mScope(mGroupMutation, "SCOPE", "Mutation scope: 'commit' (changed lines only) or 'all' (entire codebase)",
           {'s', "scope"}),
    mPatterns(mGroupMutation, "EXPR",
              "Glob patterns to constrain mutation scope; prefix with ! to exclude (repeatable)",
              {'p', "pattern"}),
    mExtensions(mGroupMutation, "EXT", "Source file extensions to mutate", {"extension"}),
    mGenerator(mGroupMutation, "TYPE", "Mutant selection strategy: uniform, random, weighted", {"generator"}),
    mSeed(mGroupMutation, "N", "Random seed for mutant selection (default: random)", {"seed"}),
    mOperators(mGroupMutation, "OP",
               "Mutation operators to apply (default: all). OP=AOR, BOR, LCR, ROR, SDL, SOR, UOI", {"operator"}),
    mLimit(mGroupAdvanced, "N", "Maximum number of mutants to generate (0 = unlimited)", {'l', "limit"}),
    mCoverageFiles(mGroupAdvanced, "FILE", "lcov coverage info file; limits mutation to covered lines only",
                   {"coverage"}),
    mPartition(mGroupAdvanced, "N/TOTAL", "Evaluate only the N-th slice of the full mutant list out of TOTAL",
               {"partition"}),
    mThreshold(mGroupAdvanced, "PCT", "Fail with exit code 3 if mutation score is below this percentage (0.0-100.0)",
               {"threshold"}) {
}

void CliConfigParser::applyTo(Config* cfg) {
  namespace fs = std::filesystem;

  if (mSourceDir) cfg->sourceDir = fs::absolute(mSourceDir.Get()).lexically_normal();
  if (mWorkDir) cfg->workDir = fs::absolute(mWorkDir.Get()).lexically_normal();
  if (mOutputDir) cfg->outputDir = fs::absolute(mOutputDir.Get()).lexically_normal();
  if (mCompileDbDir) cfg->compileDbDir = fs::absolute(mCompileDbDir.Get()).lexically_normal();
  if (mTestResultDir) cfg->testResultDir = fs::absolute(mTestResultDir.Get()).lexically_normal();

  if (mBuildCmd) cfg->buildCmd = mBuildCmd.Get();
  if (mTestCmd) cfg->testCmd = mTestCmd.Get();
  if (mTestResultExts) cfg->testResultExts = mTestResultExts.Get();
  if (mTimeout) cfg->timeout = std::stoul(mTimeout.Get());
  if (mKillAfter) cfg->killAfter = std::stoul(mKillAfter.Get());

  if (mScope) cfg->scope = mScope.Get();
  if (mExtensions) cfg->extensions = mExtensions.Get();
  if (mPatterns) cfg->patterns = mPatterns.Get();
  if (mGenerator) cfg->generator = mGenerator.Get();
  if (mOperators) cfg->operators = mOperators.Get();
  if (mCoverageFiles) {
    cfg->coverageFiles.clear();
    for (const auto& f : mCoverageFiles.Get()) {
      cfg->coverageFiles.push_back(fs::absolute(f).lexically_normal());
    }
  }

  if (mLimit) cfg->limit = mLimit.Get();
  if (mSeed) cfg->seed = static_cast<unsigned int>(std::stoul(mSeed.Get()));
  if (mThreshold) cfg->threshold = mThreshold.Get();
  if (mPartition) cfg->partition = mPartition.Get();

  cfg->init = mInit;
  cfg->dryRun = mDryRun;
  cfg->noTTY = mNoTTY;
  cfg->verbose = mVerbose;
  cfg->force = mForce;
  cfg->clean = mClean;
}

}  // namespace sentinel
