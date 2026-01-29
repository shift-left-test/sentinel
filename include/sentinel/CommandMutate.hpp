/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMANDMUTATE_HPP_
#define INCLUDE_SENTINEL_COMMANDMUTATE_HPP_

#include <string>
#include <vector>
#include "sentinel/Command.hpp"

namespace sentinel {

/**
 * @brief sentinel commandline 'mutate' subcommand class
 */
class CommandMutate : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandMutate(args::Subparser& parser);

  int run() override;

 private:
  args::ValueFlag<std::string> mMutantStr;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDMUTATE_HPP_
