/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CLICONFIGPARSER_HPP_
#define INCLUDE_SENTINEL_CLICONFIGPARSER_HPP_

#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include <args/args.hxx>
#include "sentinel/Config.hpp"

namespace sentinel {

/**
 * @brief Defines and parses Command Line Interface options.
 */
class CliConfigParser {
 public:
  /**
   * @brief Constructor - registers all options to the parser.
   * @param parser The argument parser to register options with.
   */
  explicit CliConfigParser(args::ArgumentParser& parser);  // NOLINT

  /**
   * @brief Apply CLI-parsed values onto an existing Config.
   *
   * Only flags explicitly set on the command line overwrite the config.
   * Path fields are resolved relative to the current working directory.
   *
   * @param cfg Config to modify in place.
   */
  void applyTo(Config* cfg);

  /**
   * @brief Apply only report-phase and control-flag CLI options onto a Config.
   *
   * Used when resuming from an existing workspace: only --output-dir,
   * --threshold, and --verbose are applied. All other options are ignored
   * because the workspace already holds the authoritative configuration.
   *
   * @param cfg Config to modify in place.
   */
  void applyReportOnlyTo(Config* cfg);

  /**
   * @brief Return the list of CLI option names explicitly set by the user.
   *
   * Excludes control flags (--verbose, --clean, --dry-run, --init, --force),
   * workspace/config selectors (--workspace, --config), report-phase options
   * (--output-dir, --threshold), and merge mode (--merge-partition).
   *
   * @return Vector of option name strings (e.g., "--from", "--timeout").
   */
  std::vector<std::string> getEffectiveCliOptions() const;

  /** @brief Returns the config file path if --config was specified (empty if not). */
  std::filesystem::path getConfigFile() { return mConfigFile ? mConfigFile.Get() : ""; }

  /** @brief Returns true if --init was specified. */
  bool isInit() const { return mInit; }
  /** @brief Returns true if --dry-run was specified. */
  bool isDryRun() const { return mDryRun; }
  /** @brief Returns true if --clean was specified. */
  bool isClean() const { return mClean; }
  /** @brief Returns true if --force was specified. */
  bool isForce() const { return mForce; }
  /** @brief Returns the workspace path if --workspace was specified (empty if not). */
  std::filesystem::path getWorkDir() { return mWorkDir ? std::filesystem::absolute(mWorkDir.Get()) : ""; }

 private:
  // Groups
  /** @brief Argument group for runtime control options. */
  args::Group mGroupRunCtrl;
  /** @brief Argument group for setup options. */
  args::Group mGroupSetup;
  /** @brief Argument group for build and test options. */
  args::Group mGroupBuildTest;
  /** @brief Argument group for mutation strategy options. */
  args::Group mGroupMutation;
  /** @brief Argument group for advanced options. */
  args::Group mGroupAdvanced;

  // Run options
  /** @brief Path to the configuration file. */
  args::ValueFlag<std::filesystem::path> mConfigFile;
  /** @brief Command line flag for working directory. */
  args::ValueFlag<std::filesystem::path> mWorkDir;
  /** @brief Command line flag to clear workspace and start fresh. */
  args::Flag mClean;
  /** @brief Command line flag for output directory. */
  args::ValueFlag<std::filesystem::path> mOutputDir;
  /** @brief Command line flag for dry run. */
  args::Flag mDryRun;
  /** @brief Command line flag for verbose mode. */
  args::Flag mVerbose;
  // Setup options
  /** @brief Command line flag to initialize the project. */
  args::Flag mInit;
  /** @brief Command line flag to force operations. */
  args::Flag mForce;

  // Build & test options
  /** @brief Command line flag for source directory. */
  args::ValueFlag<std::filesystem::path> mSourceDir;
  /** @brief Command line flag for build command. */
  args::ValueFlag<std::string> mBuildCmd;
  /** @brief Command line flag for compilation database directory. */
  args::ValueFlag<std::filesystem::path> mCompileDbDir;
  /** @brief Command line flag for test command. */
  args::ValueFlag<std::string> mTestCmd;
  /** @brief Command line flag for test result directory. */
  args::ValueFlag<std::filesystem::path> mTestResultDir;
  /** @brief Command line flag for test time limit. */
  args::ValueFlag<std::size_t> mTimeout;

  // Mutation options
  /** @brief Command line flag for diff base revision. */
  args::ValueFlag<std::string> mFrom;
  /** @brief Command line flag for uncommitted changes. */
  args::Flag mUncommitted;
  /** @brief Command line flag for inclusion patterns. */
  args::ValueFlagList<std::string> mPatterns;
  /** @brief Command line flag for source file extensions. */
  args::ValueFlagList<std::string> mExtensions;
  /** @brief Command line flag for mutant generator. */
  args::ValueFlag<std::string> mGenerator;
  /** @brief Command line flag for mutants per line. */
  args::ValueFlag<std::size_t> mMutantsPerLine;
  /** @brief Command line flag for random seed. */
  args::ValueFlag<unsigned int> mSeed;
  /** @brief Command line flag for mutation operators. */
  args::ValueFlagList<std::string> mOperators;
  /** @brief Command line flag for the maximum number of Clang parsers running in parallel. */
  args::ValueFlag<std::size_t> mParallelParsers;

  // Advanced options
  /** @brief Command line flag for mutant limit. */
  args::ValueFlag<std::size_t> mLimit;
  /** @brief Command line flag for lcov tracefiles. */
  args::ValueFlagList<std::filesystem::path> mLcovTracefiles;
  /** @brief Command line flag to restrict mutant generation to lcov-covered lines. */
  args::Flag mRestrict;
  /** @brief Command line flag for execution partition. */
  args::ValueFlag<std::string> mPartition;
  /** @brief Command line flag for merge partition workspaces. */
  args::ValueFlagList<std::filesystem::path> mMergePartitions;
  /** @brief Command line flag for mutation threshold. */
  args::ValueFlag<double> mThreshold;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CLICONFIGPARSER_HPP_
