/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <sstream>
#include "sentinel/Command.hpp"
#include "sentinel/Logger.hpp"

namespace sentinel {

const char* cCommandLoggerName = "Command";

Command::Command(args::Subparser& parser) :
    mSourceRoot(parser, "SOURCE_ROOT_PATH", "source root directory.", "."),
    mIsVerbose(parser, "verbose", "Show verbose messages (INFO and above).", {'v', "verbose"}),
    mIsDebug(parser, "debug", "Show debug messages (all log levels).", {"debug"}),
    mWorkDir(parser, "PATH", "Sentinel temporary working directory.", {'w', "work-dir"}, "./sentinel_tmp"),
    mOutputDir(parser, "PATH", "Directory for saving output.", {'o', "output-dir"}, "") {}

void Command::init() {
  if (mIsDebug.Get()) {
    sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::DEBUG);
  } else if (mIsVerbose.Get()) {
    sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::VERBOSE);
  }
}

}  // namespace sentinel
