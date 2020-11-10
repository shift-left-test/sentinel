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

#ifndef INCLUDE_SENTINEL_COMMAND_HPP_
#define INCLUDE_SENTINEL_COMMAND_HPP_

#include <experimental/filesystem>
#include <args/args.hxx>
#include <string>


namespace sentinel {

/**
 * @brief Command interface
 */
class Command {
 public:
  /**
   * @brief constructor
   */
  explicit Command(args::Subparser& parser);
  /**
   * @brief destructor
   */
  virtual ~Command() = default;

 public:
  /**
   * @brief initialize execution environment
   */
  void init();

  /**
   * @brief Execute subcommand.
   * 
   * @return exit code
   */
  virtual int run() = 0;

 protected:
  /**
   * @brief source root directory
   */
  args::Positional<std::string> mSourceRoot;
  /**
   * @brief verbose option
   */
  args::Flag mIsVerbose;
  /**
   * @brief internal working directory
   */
  args::ValueFlag<std::string> mWorkDir;
  /**
   * @brief output directory
   */
  args::ValueFlag<std::string> mOutputDir;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMAND_HPP_
