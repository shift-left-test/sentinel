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
    mGroupBuildTest(parser, "Build & test options:"),
    mGroupMutation(parser, "Mutation options:"),
    mConfigFile(mGroupRunCtrl, "PATH", "Path to a YAML config file (default: sentinel.yaml in current directory)",
                {"config"}),
    mOutputDir(mGroupRunCtrl, "PATH", "Directory for output reports.", {'o', "output-dir"}),
    mWorkDir(mGroupRunCtrl, "PATH", "Workspace directory for all sentinel run artifacts.", {"workspace"}),
    mVerbose(mGroupRunCtrl, "verbose", "Show build/test subprocess output and enable verbose logging.", {"verbose"}),
    mForce(mGroupRunCtrl, "force", "Skip all prompts and start fresh.", {"force"}),
    mInit(mGroupRunCtrl, "init", "Write a sentinel.yaml config template to the current directory and exit", {"init"}),
    mDryRun(mGroupRunCtrl, "dry-run",
            "Build, test, and generate mutants then exit; prints a readiness summary without evaluating mutants",
            {"dry-run"}),
    mThreshold(mGroupRunCtrl, "PCT", "Fail with exit code 3 if mutation score is below this percentage (0–100)",
               {"threshold"}),
    mNoStatusLine(mGroupRunCtrl, "no-statusline", "Disable the live status line shown in TTY mode", {"no-statusline"}),
    mSourceDir(mGroupBuildTest, "PATH", "Path to the root of the source tree.", {"source-dir"}),
    mBuildCmd(mGroupBuildTest, "CMD", "Shell command to build the project", {"build-command"}),
    mCompileDbDir(mGroupBuildTest, "PATH", "Path to the directory containing compile_commands.json.",
                  {"compiledb-dir"}),
    mTestCmd(mGroupBuildTest, "CMD", "Shell command to run tests", {"test-command"}),
    mTestResultDir(mGroupBuildTest, "PATH", "Path to the test report directory", {"test-result-dir"}),
    mTestResultExts(mGroupBuildTest, "EXT", "File extension of the test report", {"test-result-ext"}),
    mTimeout(mGroupBuildTest, "SEC", "Test time limit in seconds; 0 = no limit, auto = 2x original test time",
             {"timeout"}),
    mKillAfter(mGroupBuildTest, "SEC", "Seconds to wait after timeout before sending SIGKILL (0 = disabled)",
               {"kill-after"}),
    mScope(mGroupMutation, "SCOPE", "Mutation scope: 'commit' (changed lines only) or 'all' (entire codebase)",
           {'s', "scope"}),
    mExtensions(mGroupMutation, "EXT", "Source file extensions to mutate", {'t', "extension"}),
    mPatterns(mGroupMutation, "EXPR", "Paths or glob patterns to constrain the mutation scope", {'p', "pattern"}),
    mExcludes(mGroupMutation, "EXPR", "Exclude files/directories matching fnmatch-style patterns", {'e', "exclude"}),
    mLimit(mGroupMutation, "N", "Maximum number of mutants to generate (0 = unlimited)", {'l', "limit"}),
    mGenerator(mGroupMutation, "TYPE", "Mutant selection strategy: uniform, random, weighted", {"generator"}),
    mSeed(mGroupMutation, "N", "Random seed for mutant selection ('auto' = pick randomly)", {"seed"}),
    mOperators(mGroupMutation, "OP", "Mutation operators to apply (default: all). OP=AOR, BOR, LCR, ROR, SDL, SOR, UOI",
               {"operator"}),
    mCoverageFiles(mGroupMutation, "FILE", "lcov coverage info file; limits mutation to covered lines only",
                   {"coverage"}),
    mPartition(mGroupMutation, "N/TOTAL", "Evaluate only the N-th slice of the full mutant list out of TOTAL",
               {"partition"}) {
}

Config CliConfigParser::getConfig() {
  Config cfg;

  if (mSourceDir) cfg.sourceDir = mSourceDir.Get();
  if (mWorkDir) cfg.workDir = mWorkDir.Get();
  if (mOutputDir) cfg.outputDir = mOutputDir.Get();
  if (mVerbose) cfg.verbose = true;
  if (mForce) cfg.force = true;

  if (mBuildCmd) cfg.buildCmd = mBuildCmd.Get();
  if (mCompileDbDir) cfg.compileDbDir = mCompileDbDir.Get();
  if (mTestCmd) cfg.testCmd = mTestCmd.Get();
  if (mTestResultDir) cfg.testResultDir = mTestResultDir.Get();
  if (mTestResultExts) cfg.testResultExts = mTestResultExts.Get();
  if (mTimeout) cfg.timeout = mTimeout.Get();
  if (mKillAfter) cfg.killAfter = mKillAfter.Get();

  if (mScope) cfg.scope = mScope.Get();
  if (mExtensions) cfg.extensions = mExtensions.Get();
  if (mPatterns) cfg.patterns = mPatterns.Get();
  if (mExcludes) cfg.excludes = mExcludes.Get();
  if (mLimit) cfg.limit = mLimit.Get();
  if (mGenerator) cfg.generator = mGenerator.Get();
  if (mSeed) {
    std::string s = mSeed.Get();
    if (s != "auto") {
      cfg.seed = static_cast<unsigned int>(std::stoul(s));
    }
  }
  if (mOperators) cfg.operators = mOperators.Get();
  if (mCoverageFiles) cfg.coverageFiles = mCoverageFiles.Get();
  if (mThreshold) cfg.threshold = mThreshold.Get();
  if (mPartition) cfg.partition = mPartition.Get();

  cfg.init = mInit;
  cfg.dryRun = mDryRun;
  cfg.noStatusLine = mNoStatusLine;

  return cfg;
}

}  // namespace sentinel
