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
    mInit(mGroupSetup, "init", "Write a sentinel.yaml config template to the current directory and exit", {"init"}),
    mForce(mGroupSetup, "force", "Force overwrite of existing files.", {"force"}),
    mSourceDir(mGroupBuildTest, "PATH", "Path to the root of the source tree.", {"source-dir"}),
    mBuildCmd(mGroupBuildTest, "CMD", "Shell command to build the project", {"build-command"}),
    mCompileDbDir(mGroupBuildTest, "PATH", "Path to the directory containing compile_commands.json.",
                  {"compiledb-dir"}),
    mTestCmd(mGroupBuildTest, "CMD", "Shell command to run tests", {"test-command"}),
    mTestResultDir(mGroupBuildTest, "PATH", "Path to the test report directory", {"test-result-dir"}),
    mTimeout(mGroupBuildTest, "SEC", "Test time limit in seconds; 0 = no limit (default: 1.5x original test time)",
             {"timeout"}),
    mFrom(mGroupMutation, "REV",
          "Diff base revision (e.g., HEAD~1, main, v1.0). "
          "Mutates only lines changed between the merge-base of REV and HEAD.",
          {"from"}),
    mUncommitted(mGroupMutation, "uncommitted",
                 "Include uncommitted changes (staged + unstaged + untracked) in mutation scope.",
                 {"uncommitted"}),
    mPatterns(mGroupMutation, "EXPR",
              "Glob patterns to constrain mutation scope; prefix with ! to exclude (repeatable)",
              {'p', "pattern"}),
    mExtensions(mGroupMutation, "EXT", "Source file extensions to mutate", {"extension"}),
    mGenerator(mGroupMutation, "TYPE", "Mutant selection strategy: uniform, random, weighted", {"generator"}),
    mMutantsPerLine(mGroupMutation, "N", "Maximum number of mutants per source line (0 = unlimited)",
                    {"mutants-per-line"}),
    mSeed(mGroupMutation, "N", "Random seed for mutant selection (default: random)", {"seed"}),
    mOperators(mGroupMutation, "OP",
               "Mutation operators to apply (default: all). OP=AOR, BOR, LCR, ROR, SDL, SOR, UOI", {"operator"}),
    mLimit(mGroupAdvanced, "N", "Maximum number of mutants to generate (0 = unlimited)", {'l', "limit"}),
    mLcovTracefiles(mGroupAdvanced, "FILE", "skip evaluation for uncovered mutants",
                    {"lcov-tracefile"}),
    mRestrict(mGroupAdvanced, "restrict",
              "Restrict mutant generation to lines covered by --lcov-tracefile "
              "(otherwise uncovered mutants are kept in the report as SURVIVED*).",
              {"restrict"}),
    mPartition(mGroupAdvanced, "N/TOTAL", "Evaluate only the N-th slice of the full mutant list out of TOTAL",
               {"partition"}),
    mMergePartitions(mGroupAdvanced, "PATH",
                     "Merge a partitioned workspace result into the target workspace (repeatable)",
                     {"merge-partition"}),
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
  if (mTimeout) cfg->timeout = mTimeout.Get();

  if (mFrom) cfg->from = mFrom.Get();
  cfg->uncommitted = mUncommitted;
  if (mExtensions) cfg->extensions = mExtensions.Get();
  if (mPatterns) cfg->patterns = mPatterns.Get();
  if (mGenerator) cfg->generator = parseGenerator(mGenerator.Get());
  if (mMutantsPerLine) cfg->mutantsPerLine = mMutantsPerLine.Get();
  if (mOperators) cfg->operators = mOperators.Get();
  if (mLcovTracefiles) {
    cfg->lcovTracefiles.clear();
    for (const auto& f : mLcovTracefiles.Get()) {
      cfg->lcovTracefiles.push_back(fs::absolute(f).lexically_normal());
    }
  }
  if (mRestrict) cfg->restrictGeneration = true;

  if (mLimit) cfg->limit = mLimit.Get();
  if (mSeed) cfg->seed = mSeed.Get();
  if (mThreshold) cfg->threshold = mThreshold.Get();
  if (mPartition) cfg->partition = mPartition.Get();
  if (mMergePartitions) {
    cfg->mergeWorkspaces.clear();
    for (const auto& p : mMergePartitions.Get()) {
      cfg->mergeWorkspaces.push_back(fs::absolute(p).lexically_normal());
    }
  }

  cfg->init = mInit;
  cfg->dryRun = mDryRun;
  cfg->verbose = mVerbose;
  cfg->force = mForce;
  cfg->clean = mClean;
}

std::vector<std::string> CliConfigParser::getEffectiveCliOptions() const {
  std::vector<std::string> opts;
  if (mSourceDir) opts.push_back("--source-dir");
  if (mBuildCmd) opts.push_back("--build-command");
  if (mCompileDbDir) opts.push_back("--compiledb-dir");
  if (mTestCmd) opts.push_back("--test-command");
  if (mTestResultDir) opts.push_back("--test-result-dir");
  if (mTimeout) opts.push_back("--timeout");
  if (mFrom) opts.push_back("--from");
  if (mUncommitted) opts.push_back("--uncommitted");
  if (mPatterns) opts.push_back("--pattern");
  if (mExtensions) opts.push_back("--extension");
  if (mGenerator) opts.push_back("--generator");
  if (mMutantsPerLine) opts.push_back("--mutants-per-line");
  if (mSeed) opts.push_back("--seed");
  if (mOperators) opts.push_back("--operator");
  if (mLcovTracefiles) opts.push_back("--lcov-tracefile");
  if (mRestrict) opts.push_back("--restrict");
  if (mLimit) opts.push_back("--limit");
  if (mPartition) opts.push_back("--partition");
  return opts;
}

void CliConfigParser::applyReportOnlyTo(Config* cfg) {
  namespace fs = std::filesystem;
  if (mOutputDir) cfg->outputDir = fs::absolute(mOutputDir.Get()).lexically_normal();
  if (mThreshold) cfg->threshold = mThreshold.Get();
  cfg->verbose = mVerbose;
  cfg->force = mForce;
  cfg->clean = mClean;
  cfg->dryRun = mDryRun;
  cfg->init = mInit;
}

}  // namespace sentinel
