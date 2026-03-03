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

Command::Command(args::Group& parser) :
    mSourceRoot(parser, "SOURCE_ROOT_PATH", "Root of the source tree to test.", "."),
    mIsVerbose(parser, "verbose", "Enable verbose output (INFO level and above).", {"verbose"}),
    mSilent(parser, "silent", "Suppress build/test subprocess output to the terminal.", {"silent"}),
    mIsDebug(parser, "debug", "Enable debug output (all log levels).", {"debug"}),
    mWorkDir(parser, "PATH", "Directory for all run artifacts.", {'w', "workspace"}, "./sentinel_workspace"),
    mOutputDir(parser, "PATH", "Directory to write HTML/XML reports.", {'o', "output-dir"}, ""),
    mCwd(parser, "PATH", "Change to this directory before running.", {"cwd"}, "") {}

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
