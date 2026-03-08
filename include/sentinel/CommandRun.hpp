/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMANDRUN_HPP_
#define INCLUDE_SENTINEL_COMMANDRUN_HPP_

#include <cstddef>
#include <experimental/filesystem>  // NOLINT(build/include_order)
#include <optional>
#include <string>
#include <vector>
#include "sentinel/Command.hpp"
#include "sentinel/SentinelConfig.hpp"

namespace sentinel {

/**
 * @brief Runs the full mutation testing pipeline in standalone mode.
 *
 * Orchestrates all phases: build, baseline test, mutant generation,
 * per-mutant build/test/evaluate, and report generation.
 */
class CommandRun : public Command {
 public:
  /**
   * @brief Constructor — registers all command-line options with @p parser.
   */
  explicit CommandRun(args::Group& parser);  // NOLINT(runtime/references)

  /**
   * @brief Loads the YAML config file and applies log-level settings.
   *
   * Called before run(). Handles @c --init early exit.
   */
  void init() override;

  /**
   * @brief Executes the mutation testing pipeline.
   *
   * @return 0 on success, non-zero on error.
   */
  int run() override;

  /**
   * @brief Restores source files modified by a mutant from the backup directory.
   *
   * @param backup  Path to the backup directory containing original files.
   * @param srcRoot Path to the source root where files are restored.
   */
  static void restoreBackup(const std::string& backup, const std::string& srcRoot);

 protected:
  /**
   * @brief Installs signal handlers that restore the backup on abnormal exit.
   */
  virtual void setSignalHandler();

  /**
   * @brief Set by init() when --config causes a working-directory change.
   *
   * Holds the absolute path of the new working directory so that run() can
   * include it in the pre-run configuration warning output.
   */
  std::optional<std::string> mChangedToDir;

  /**
   * @brief Option group for run-control flags (--config, --init, --no-statusline).
   */
  args::Group mGroupRunCtrl;

  /**
   * @brief Option group for build and test flags.
   */
  args::Group mGroupBuildTest;

  /**
   * @brief Option group for mutation-related flags.
   */
  args::Group mGroupMutation;

  /**
   * @brief Path to the YAML configuration file supplied via --config.
   */
  args::ValueFlag<std::string> mConfigFile;

  /**
   * @brief Configuration loaded from the YAML file; populated in init().
   */
  std::optional<SentinelConfig> mConfig;

  /**
   * @brief Copies test result files with the given extensions from @p from to @p to.
   *
   * @param from Source directory containing test result files.
   * @param to   Destination directory to copy files into.
   * @param exts File extensions to copy (e.g. {"xml", "XML"}).
   */
  void copyTestReportTo(const std::string& from, const std::string& to, const std::vector<std::string>& exts);

  /**
   * @brief Ensures @p target exists as a directory and returns its canonical path.
   *
   * @param target      Directory path to prepare.
   * @param targetExists Set to @c true if the directory already existed, @c false otherwise.
   * @param isFilledDir  When @c false, throws if the directory is non-empty.
   * @return Canonical absolute path of @p target.
   */
  std::string preProcessWorkDir(const std::string& target, bool* targetExists, bool isFilledDir);

  /**
   * @brief Returns the resolved source root path (CLI > YAML > default).
   */
  virtual std::string getSourceDir();

  /**
   * @brief Returns the directory that contains compile_commands.json.
   */
  virtual std::string getCompileDbDir();

  /**
   * @brief Returns the resolved workspace directory path.
   */
  virtual std::string getWorkDir();

  /**
   * @brief Returns the resolved report output directory path.
   */
  virtual std::string getOutputDir();

  /**
   * @brief Returns the directory where the test command writes result files.
   */
  virtual std::string getTestResultDir();

  /**
   * @brief Returns the shell command used to build the project.
   */
  virtual std::string getBuildCmd();

  /**
   * @brief Returns the shell command used to run the test suite.
   */
  virtual std::string getTestCmd();

  /**
   * @brief Returns the mutant selection strategy name (uniform/random/weighted).
   */
  virtual std::string getGenerator();

  /**
   * @brief Returns the list of test result file extensions to collect.
   */
  virtual std::vector<std::string> getTestResultFileExts();

  /**
   * @brief Returns the list of source file extensions eligible for mutation.
   */
  virtual std::vector<std::string> getTargetFileExts();

  /**
   * @brief Returns the list of paths or glob patterns that constrain the mutation scope.
   */
  virtual std::vector<std::string> getPatterns();

  /**
   * @brief Returns the list of fnmatch-style patterns for paths excluded from mutation.
   */
  virtual std::vector<std::string> getExcludePaths();

  /**
   * @brief Returns the list of lcov coverage info files.
   */
  virtual std::vector<std::string> getCoverageFiles();

  /**
   * @brief Returns the mutation scope ("commit" or "all").
   */
  virtual std::string getScope();

  /**
   * @brief Returns the maximum number of mutants to generate.
   */
  virtual size_t getMutantLimit();

  /**
   * @brief Returns the test time limit string ("auto", "0", or a positive integer).
   */
  virtual std::string getTestTimeLimit();

  /**
   * @brief Returns the post-timeout SIGKILL delay in seconds as a string.
   */
  virtual std::string getKillAfter();

  /**
   * @brief Returns the random seed for mutant selection.
   */
  virtual unsigned getSeed();

  /**
   * @brief Returns the list of mutation operator names to apply.
   *
   * An empty list means all operators are used.
   */
  virtual std::vector<std::string> getOperators();

  /**
   * @brief Returns whether verbose output is enabled.
   */
  virtual bool getVerbose();

  /**
   * @brief Returns the minimum mutation score threshold as a percentage (0–100),
   *        or @c std::nullopt if no threshold was specified.
   *
   * When set, @c run() returns exit code 3 if the final mutation score is strictly
   * below this value.
   */
  virtual std::optional<double> getThreshold();

  // Run control options (registered to mGroupRunCtrl)

  /** @brief Flag: write a sentinel.yaml template and exit. */
  args::Flag mInit;

  /**
   * @brief Flag: perform only the build, test, and mutant-generation phases,
   *        then print a readiness summary and exit without evaluating any mutants.
   */
  args::Flag mDryRun;

  /**
   * @brief Minimum mutation score threshold (0–100).
   *
   * When provided via @c --threshold, @c run() emits a summary line to stderr and
   * returns exit code 3 if the final mutation score falls strictly below this value.
   */
  args::ValueFlag<double> mThreshold;

  /** @brief Flag: disable the live terminal status line. */
  args::Flag mNoStatusLine;

  // Build & test options (registered to mGroupBuildTest)

  /** @brief Path to the root of the source tree. */
  args::ValueFlag<std::string> mSourceDir;

  /** @brief Shell command to build the project. */
  args::ValueFlag<std::string> mBuildCmd;

  /** @brief Directory containing compile_commands.json. */
  args::ValueFlag<std::string> mCompileDbDir;

  /** @brief Shell command to run the test suite. */
  args::ValueFlag<std::string> mTestCmd;

  /** @brief Path to the test report directory. */
  args::ValueFlag<std::string> mTestResultDir;

  /** @brief File extension of the test report. */
  args::ValueFlagList<std::string> mTestResultFileExts;

  /** @brief Test time limit as a string ("auto", "0", or seconds). */
  args::ValueFlag<std::string> mTimeLimitStr;

  /** @brief Seconds to wait after timeout before sending SIGKILL. */
  args::ValueFlag<std::string> mKillAfterStr;

  // Mutation options (registered to mGroupMutation)

  /** @brief Mutation scope: "commit" or "all". */
  args::ValueFlag<std::string> mScope;

  /** @brief Source file extensions to mutate. */
  args::ValueFlagList<std::string> mExtensions;

  /** @brief Paths or glob patterns to constrain the mutation scope. */
  args::ValueFlagList<std::string> mPatterns;

  /** @brief fnmatch-style patterns for paths excluded from mutation. */
  args::ValueFlagList<std::string> mExcludes;

  /** @brief Maximum number of mutants to generate. */
  args::ValueFlag<size_t> mLimit;

  /** @brief Mutant selection strategy: "uniform", "random", or "weighted". */
  args::ValueFlag<std::string> mGenerator;

  /** @brief Random seed for mutant selection ('auto' = pick randomly). */
  args::ValueFlag<std::string> mSeed;

  /** @brief Mutation operators to apply; empty means all operators. */
  args::ValueFlagList<std::string> mOperators;

  /** @brief lcov coverage info files; limits mutation to covered lines. */
  args::ValueFlagList<std::string> mCoverageFiles;

 private:
  /**
   * @brief Checks resolved options for potentially problematic settings and
   *        asks the user to confirm before starting the pipeline.
   *
   * @param sourceRoot    Canonical path to the source root.
   * @param mutantLimit   Maximum number of mutants (0 = unlimited).
   * @param timeLimitStr  Raw timeout string from CLI/config ("auto", "0", or integer).
   * @param diffPatterns  Resolved --pattern values.
   * @param excludePaths  Resolved --exclude values.
   * @return @c true if the run should proceed, @c false if the user aborted.
   */
  bool checkConfigWarnings(
      const std::experimental::filesystem::path& sourceRoot,
      size_t mutantLimit,
      const std::string& timeLimitStr,
      const std::vector<std::string>& diffPatterns,
      const std::vector<std::string>& excludePaths);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDRUN_HPP_
