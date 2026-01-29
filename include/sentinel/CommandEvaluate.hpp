/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMANDEVALUATE_HPP_
#define INCLUDE_SENTINEL_COMMANDEVALUATE_HPP_

#include <string>
#include <vector>
#include "sentinel/Command.hpp"

namespace sentinel {

/**
 * @brief sentinel commandline 'evaluate' subcommand class
 */
class CommandEvaluate : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandEvaluate(args::Subparser& parser);

  int run() override;

 private:
  args::ValueFlag<std::string> mMutantStr;
  args::ValueFlag<std::string> mExpectedDir;
  args::ValueFlag<std::string> mActualDir;
  args::ValueFlag<std::string> mEvalFile;
  args::ValueFlag<std::string> mTestState;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDEVALUATE_HPP_
