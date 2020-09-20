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

#ifndef INCLUDE_SENTINEL_REPORT_HPP_
#define INCLUDE_SENTINEL_REPORT_HPP_

#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/MutationResults.hpp"

namespace sentinel {

/**
 * @brief Report interface
 */
class Report {
 public:
  /**
   * @brief Default Constructor
   *
   * @param resultsPath directory path of mutation results
   * @param sourcePath directory path of source files
   * @throw InvalidArgumentException
   *        when resultsPath is empty or sourcePath doesn't exist
   */
  Report(const std::string& resultsPath, const std::string& sourcePath);
  /**
   * @brief Default Destructor
   */
  ~Report();
  /**
   * @brief Save the report to the given path
   *
   * @param path to save the report
   */
  virtual void save(const std::string& path) = 0;

  /**
   * @brief Print summury of report
   */
  void printSummary();

 protected:
  /**
   * @brief MutationResults instance
   */
  MutationResults mResults;
  /**
   * @brief group MutationReuslt by Directory
   */
  std::map<std::string,
      std::tuple<std::vector<const MutationResult*>*, int, int, int>* >
      groupByDirPath;

  /**
   * @brief group MutationResult by File
   */
  std::map<std::string,
      std::tuple<std::vector<const MutationResult*>*, int, int>* > groupByPath;

  /**
   * @brief total Number Of Mutation
   */
  int totNumberOfMutation;

  /**
   * @brief total number of Deteced Mutation
   */
  int totNumberOfDetectedMutation;

  /**
   * @brief path of source directory
   */
  std::string mSourcePath;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_REPORT_HPP_
