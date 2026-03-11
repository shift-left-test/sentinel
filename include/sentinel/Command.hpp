/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMAND_HPP_
#define INCLUDE_SENTINEL_COMMAND_HPP_

#include <args/args.hxx>
#include <filesystem>  // NOLINT
#include <string>

namespace sentinel {

/**
 * @brief Command interface
 */
class Command {
 public:
  /**
   * @brief constructor
   */
  explicit Command(args::Group& parser);
  /**
   * @brief destructor
   */
  virtual ~Command() = default;

 public:
  /**
   * @brief initialize execution environment
   */
  virtual void init();

  /**
   * @brief Execute subcommand.
   *
   * @return exit code
   */
  virtual int run() = 0;

 protected:
  /**
   * @brief output directory
   */
  args::ValueFlag<std::string> mOutputDir;

  /**
   * @brief internal working directory
   */
  args::ValueFlag<std::string> mWorkDir;

  /**
   * @brief verbose option
   */
  args::Flag mIsVerbose;

  /**
   * @brief silent option — suppress build/test subprocess output to terminal
   */
  args::Flag mSilent;

  /**
   * @brief debug option
   */
  args::Flag mIsDebug;

  /**
   * @brief force option — skip all prompts and start fresh
   */
  args::Flag mForce;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMAND_HPP_
