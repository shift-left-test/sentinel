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

#ifndef INCLUDE_SENTINEL_COMMANDREPORT_HPP_
#define INCLUDE_SENTINEL_COMMANDREPORT_HPP_

#include <string>
#include <vector>
#include <CLI11.hpp>
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
  explicit CommandReport(CLI::App* app);

  int run(const std::string& sourceRoot,
    const std::string& workDir, const std::string& outputDir,
    bool verbose) override;

 private:
  std::string mEvalFile;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDREPORT_HPP_