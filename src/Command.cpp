/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "sentinel/Command.hpp"
#include "sentinel/Logger.hpp"

namespace sentinel {

const char* cCommandLoggerName = "Command";

Command::Command(args::Subparser& parser) :
    mSourceRoot(parser, "SOURCE_ROOT_PATH", "source root directory.", "."),
    mIsVerbose(parser, "verbose", "Show verbose messages (INFO and above).", {'v', "verbose"}),
    mIsDebug(parser, "debug", "Show debug messages (all log levels).", {"debug"}),
    mWorkDir(parser, "PATH", "Sentinel workspace directory for all run artifacts.", {'w', "workspace"}, "./sentinel_workspace"),
    mOutputDir(parser, "PATH", "Directory for saving output.", {'o', "output-dir"}, ""),
    mCwd(parser, "PATH", "Change the current working directory before running.", {"cwd"}, "") {}

void Command::init() {
  namespace fs = std::experimental::filesystem;

  if (!mCwd.Get().empty()) {
    std::error_code ec;
    fs::current_path(mCwd.Get(), ec);
    if (ec) {
      throw std::runtime_error(fmt::format("Failed to change directory to '{}': {}", mCwd.Get(), ec.message()));
    }
  }

  if (mIsDebug.Get()) {
    sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::DEBUG);
  } else if (mIsVerbose.Get()) {
    sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::VERBOSE);
  }
}

}  // namespace sentinel
