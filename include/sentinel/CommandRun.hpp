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

#ifndef INCLUDE_SENTINEL_COMMANDRUN_HPP_
#define INCLUDE_SENTINEL_COMMANDRUN_HPP_

#include <cstddef>
#include <string>
#include <vector>
#include "sentinel/Command.hpp"


namespace sentinel {

/**
 * @brief sentinel commandline 'run' subcommand class
 */
class CommandRun : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandRun(args::Subparser& parser);

  int run() override;

 private:
  void copyTestReportTo(const std::string& from,
      const std::string& to, const std::vector<std::string>& exts);
  void restoreBackup(const std::string& backup, const std::string& srcRoot);
  std::string preProcessWorkDir(const std::string& target, bool* targetExists,
      bool isFilledDir);
  int executeCmdWithTimeOut(const std::string& cmd, std::size_t sec = 0,
      bool* occurTimeout = nullptr);

 private:
  args::ValueFlag<std::string> mBuildDir;
  args::ValueFlag<std::string> mTestResultDir;
  args::ValueFlag<std::string> mBuildCmd;
  args::ValueFlag<std::string> mTestCmd;
  args::ValueFlag<std::string> mGenerator;
  args::ValueFlagList<std::string> mTestResultFileExts;
  args::ValueFlagList<std::string> mExtensions;
  args::ValueFlagList<std::string> mExcludes;
  args::ValueFlag<std::string> mScope;
  args::ValueFlag<int> mLimit;
  args::ValueFlag<std::size_t> mTimeLimit;
  args::ValueFlag<std::size_t> mKillAfter;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDRUN_HPP_
