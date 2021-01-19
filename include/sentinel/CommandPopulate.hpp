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

#ifndef INCLUDE_SENTINEL_COMMANDPOPULATE_HPP_
#define INCLUDE_SENTINEL_COMMANDPOPULATE_HPP_

#include <string>
#include <vector>
#include "sentinel/Command.hpp"


namespace sentinel {

/**
 * @brief sentinel commandline 'populate' subcommand class
 */
class CommandPopulate : public Command {
 public:
  /**
   * @brief constructor
   */
  explicit CommandPopulate(args::Subparser& parser);

  int run() override;

 private:
  args::ValueFlag<std::string> mBuildDir;
  args::ValueFlag<std::string> mScope;
  args::ValueFlagList<std::string> mExtensions;
  args::ValueFlagList<std::string> mExcludes;
  args::ValueFlag<int> mLimit;
  args::ValueFlag<std::string> mMutableFilename;
  args::ValueFlag<std::string> mGenerator;
  args::ValueFlag<unsigned> mSeed;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDPOPULATE_HPP_
