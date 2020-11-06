/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <sstream>
#include "sentinel/Command.hpp"
#include "sentinel/Logger.hpp"


namespace sentinel {
const char * cCommandLoggerName = "Command";

Command::Command(args::Subparser& parser) :
  mSourceRoot(parser, "SOURCE_ROOT",
    "source root directory. default: .",
    "."),
  mIsVerbose(parser, "verbose", "Verbosity", {'v', "verbose"}),
  mWorkDir(parser, "work_dir",
    "Sentinel temporary working directory.",
    {'w', "work-dir"}, "./sentinel_tmp"),
  mOutputDir(parser, "output_dir",
    "Directory for saving output.",
    {'o', "output-dir"}, ".") {
}

void Command::init() {
  namespace fs =  std::experimental::filesystem;
  fs::create_directories(mWorkDir.Get());
  fs::create_directories(mOutputDir.Get());

  if (mIsVerbose.Get()) {
    sentinel::Logger::setLevel(sentinel::Logger::Level::INFO);
  }
}
}  // namespace sentinel