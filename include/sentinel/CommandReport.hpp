/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMANDREPORT_HPP_
#define INCLUDE_SENTINEL_COMMANDREPORT_HPP_

#include <string>
#include <vector>
#include "sentinel/Command.hpp"

namespace sentinel {

/**
 * @brief sentinel commandline 'report' subcommand class
 */
class CommandReport : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandReport(args::Subparser& parser);

  int run() override;

 private:
  args::ValueFlag<std::string> mEvalFile;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDREPORT_HPP_
