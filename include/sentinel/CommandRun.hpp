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
#include <experimental/filesystem>
#include <string>
#include <vector>
#include "sentinel/Command.hpp"


namespace sentinel {

extern bool workDirExists;
extern bool backupDirExists;
extern bool expectedDirExists;
extern bool actualDirExists;
extern std::experimental::filesystem::path workDirForSH;

/**
 * @brief sentinel commandline 'run' subcommand class.
 */
class CommandRun : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandRun(args::Subparser& parser);

  int run() override;

 protected:
  /**
   * @brief set signal handler
   */
  virtual void setSignalHandler();

  /**
   * @brief copy test results file with given extensions from FROM to TO
   *
   * @param from location
   * @param to location
   * @param exts extensions
   */
  void copyTestReportTo(const std::string& from,
      const std::string& to, const std::vector<std::string>& exts);

  /**
   * @brief restore changed file by copying file from backup folder
   *        to source folder
   *
   * @param backup location
   * @param srcRoot source location
   */
  void restoreBackup(const std::string& backup, const std::string& srcRoot);

  /**
   * @brief preprocess working directory
   *
   * @param target directory
   * @param targetExists or not
   * @param isFilledDir or not
   * @return absolute path of target directory 
   */
  std::string preProcessWorkDir(const std::string& target, bool* targetExists,
      bool isFilledDir);

  /**
   * @brief convert string to integer
   *
   * @param target string
   * @return converted integer value
   */
  template<typename T>
  T strToInt(const std::string& target);

  /**
   * @brief return source root path
   * @return source root path
   */
  virtual std::string getSourceRoot();

  /**
   * @brief build directory
   * @return build directory
   */
  virtual std::string getBuildDir();

  /**
   * @brief get work directory
   * @return work directory
   */
  virtual std::string getWorkDir();

  /**
   * @brief get output directory
   * @return output directory
   */
  virtual std::string getOutputDir();

  /**
   * @brief get test result directory
   * @return test result directory
   */
  virtual std::string getTestResultDir();

  /**
   * @brief get build command
   * @return build command
   */
  virtual std::string getBuildCmd();

  /**
   * @brief get test command
   * @return test command
   */
  virtual std::string getTestCmd();

  /**
   * @brief get mutant generator type
   * @return mutant generator type
   */
  virtual std::string getGenerator();

  /**
   * @brief get test result file extensions
   * @return test result file extension list
   */
  virtual std::vector<std::string> getTestResultFileExts();

  /**
   * @brief get file extensions as target for mutation
   * @return target file extension list
   */
  virtual std::vector<std::string> getTargetFileExts();

  /**
   * @brief get excluded paths
   * @return list of excluded paths
   */
  virtual std::vector<std::string> getExcludePaths();

  /**
   * @brief get mutation scope
   * @return mutation scope
   */
  virtual std::string getScope();

  /**
   * @brief get maximum number of mutant to be generated
   * @return mutant limit
   */
  virtual int getMutantLimit();

  /**
   * @brief get test timeout
   * @return test timeout
   */
  virtual std::string getTestTimeLimit();

  /**
   * @brief get duration after which SIGKILL is sent to timeout-ed test process
   * @return time after which test process is killed forcefully
   */
  virtual std::string getKillAfter();

  /**
   * @brief get random seed
   * @return random seed
   */
  virtual unsigned getSeed();

  /**
   * @brief get verbose status
   * @return verbose status
   */
  virtual bool getVerbose();

 protected:
  /**
   * @brief build directory
   */
  args::ValueFlag<std::string> mBuildDir;

  /**
   * @brief test result directory
   */
  args::ValueFlag<std::string> mTestResultDir;

  /**
   * @brief build command
   */
  args::ValueFlag<std::string> mBuildCmd;

  /**
   * @brief test command
   */
  args::ValueFlag<std::string> mTestCmd;

  /**
   * @brief mutant generator type
   */
  args::ValueFlag<std::string> mGenerator;

  /**
   * @brief extensions of test result files
   */
  args::ValueFlagList<std::string> mTestResultFileExts;

  /**
   * @brief target file extensions for mutation
   */
  args::ValueFlagList<std::string> mExtensions;

  /**
   * @brief paths excluded from mutation
   */
  args::ValueFlagList<std::string> mExcludes;

  /**
   * @brief mutation scope
   */
  args::ValueFlag<std::string> mScope;

  /**
   * @brief maximum number of mutants to be generated
   */
  args::ValueFlag<int> mLimit;

  /**
   * @brief test timeout
   */
  args::ValueFlag<std::string> mTimeLimitStr;

  /**
   * @brief time after which SIGKILL is sent to timeout-ed test process
   */
  args::ValueFlag<std::string> mKillAfterStr;

  /**
   * @brief random seed
   */
  args::ValueFlag<unsigned> mSeed;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDRUN_HPP_
