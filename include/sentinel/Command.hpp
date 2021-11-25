/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMAND_HPP_
#define INCLUDE_SENTINEL_COMMAND_HPP_

#include <experimental/filesystem>
#include <string>
#include <args/args.hxx>


namespace sentinel {

/**
 * @brief Command interface
 */
class Command {
 public:
  /**
   * @brief constructor
   */
  explicit Command(args::Subparser& parser);
  /**
   * @brief destructor
   */
  virtual ~Command() = default;

 public:
  /**
   * @brief initialize execution environment
   */
  void init();

  /**
   * @brief Execute subcommand.
   *
   * @return exit code
   */
  virtual int run() = 0;

 protected:
  /**
   * @brief source root directory
   */
  args::Positional<std::string> mSourceRoot;

  /**
   * @brief verbose option
   */
  args::Flag mIsVerbose;

  /**
   * @brief internal working directory
   */
  args::ValueFlag<std::string> mWorkDir;

  /**
   * @brief output directory
   */
  args::ValueFlag<std::string> mOutputDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMAND_HPP_
