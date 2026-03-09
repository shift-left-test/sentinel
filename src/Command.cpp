/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "sentinel/Command.hpp"
#include "sentinel/Logger.hpp"

namespace sentinel {

const char* cCommandLoggerName = "Command";

Command::Command(args::Group& parser) :
    mOutputDir(parser, "PATH", "Path to the directory to write mutation test reports (HTML/XML).",
               {'o', "output-dir"}, ""),
    mWorkDir(parser, "PATH", "Path to the workspace directory for run artifacts.",
             {'w', "workspace"}, "./sentinel_workspace"),
    mIsVerbose(parser, "verbose", "Enable verbose output (INFO level and above).", {"verbose"}),
    mSilent(parser, "silent", "Suppress build/test subprocess output to the terminal.", {"silent"}),
    mIsDebug(parser, "debug", "Enable debug output (all log levels).", {"debug"}),
    mForce(parser, "force", "Skip all prompts and start fresh, overwriting any previous state.", {'f', "force"}) {}

void Command::init() {
  if (mIsDebug.Get()) {
    sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::DEBUG);
  } else if (mIsVerbose.Get()) {
    sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::VERBOSE);
  }
}

}  // namespace sentinel
