 /*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

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

#ifndef INCLUDE_SENTINEL_HTMLREPORT_HPP_
#define INCLUDE_SENTINEL_HTMLREPORT_HPP_

#include <experimental/filesystem>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/Report.hpp"


namespace sentinel {

/**
 * @brief HTMLReport class
 */
class HTMLReport : public Report {
 public:
  /**
   * @brief Default constructor
   *
   * @param results mutation results
   * @param sourcePath directory path of source files
   */
  HTMLReport(const MutationResults& results, const std::string& sourcePath);

  /**
   * @brief Default constructor
   *
   * @param resultsPath directory path of mutation results
   * @param sourcePath directory path of source files
   */
  HTMLReport(const std::string& resultsPath, const std::string& sourcePath);

  /**
   * @brief save html format result to path
   *
   * @param dirPath
   * @throw InvalidArgumentException when path is not directory
   */
  void save(const std::experimental::filesystem::path& dirPath) override;

 private:
  /**
   * @brief makeIndexHtml
   *
   * @param totNumberOfMutation
   * @param totNumberOfDetecedMutation
   * @param root index or not
   * @param currentDirPath current key of groupByDirPath (used in non root)
   * @param outputDir
   */
  void makeIndexHtml(
      std::size_t totNumberOfMutation, std::size_t totNumberOfDetectedMutation,
      bool root, const std::experimental::filesystem::path& currentDirPath,
      const std::experimental::filesystem::path& outputDir);

  /**
   * @brief makeSourceHtml
   *
   * @param MRs Mutation Resulsts of a source file
   * @param srcPath
   * @param outputDir
   */
  void makeSourceHtml(
      std::vector<const MutationResult*>* MRs,
      const std::experimental::filesystem::path& srcPath,
      const std::experimental::filesystem::path& outputDir);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_HTMLREPORT_HPP_
