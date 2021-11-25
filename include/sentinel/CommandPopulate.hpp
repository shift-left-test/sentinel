/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMANDPOPULATE_HPP_
#define INCLUDE_SENTINEL_COMMANDPOPULATE_HPP_

#include <string>
#include <vector>
#include "sentinel/Command.hpp"


namespace sentinel {

/**
 * @brief sentinel commandline 'populate' subcommand class
 */
class CommandPopulate : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandPopulate(args::Subparser& parser);

  int run() override;

 private:
  args::ValueFlag<std::string> mBuildDir;
  args::ValueFlag<std::string> mScope;
  args::ValueFlagList<std::string> mExtensions;
  args::ValueFlagList<std::string> mExcludes;
  args::ValueFlag<int> mLimit;
  args::ValueFlag<std::string> mMutableFilename;
  args::ValueFlag<std::string> mGenerator;
  args::ValueFlag<unsigned> mSeed;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDPOPULATE_HPP_
