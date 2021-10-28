/*
  MIT License

  Copyright (c) 2021 LG Electronics, Inc.

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

#ifndef INCLUDE_SENTINEL_COVERAGEINFO_HPP_
#define INCLUDE_SENTINEL_COVERAGEINFO_HPP_

#include <map>
#include <string>
#include <vector>
#include "sentinel/SourceLine.hpp"

namespace sentinel {

/**
 * @brief CoverageInfo class
 */
class CoverageInfo {
 public:
  /**
   * @brief Default constructor
   */
  CoverageInfo() = default;

  /**
   * @brief Constructor
   *
   * @param filenames list of lcov-format coverage result file
   */
  explicit CoverageInfo(const std::vector<std::string>& filenames);

  /**
   * @brief Check if a code line is covered by test cases
   *
   * @param filename source code filename
   * @param line number
   * @return True if line is covered by test cases
   */
  bool cover(const std::string& filename, size_t line);

 private:
  /**
   * @brief map from file name to list of covered lines
   */
  std::map<std::string, std::vector<size_t>> mData;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
