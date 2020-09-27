 /*
  MIT License

  Copyright (c) 2020 Sangmo Kang

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

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_INDEXHTMLGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_INDEXHTMLGENERATOR_HPP_

#include <stddef.h>
#include <string>
#include <vector>
#include "sentinel/docGenerator/DOCGenerator.hpp"

namespace sentinel {

/**
 * @brief IndexHTMLGenerator class
 */
class IndexHTMLGenerator : public DOCGenerator {
 public:
  /**
   * @brief Default constructor
   *
   * @param root root index or not
   * @param dirName current dirName
   * @param sizeOfTargetFiles
   * @param coverage
   * @param numerator
   * @param denominator
   */
  IndexHTMLGenerator(bool root,
                     const std::string& dirName,
                     std::size_t sizeOfTargetFiles,
                     unsigned int coverage,
                     std::size_t numerator,
                     std::size_t denominator);

  /**
   * @brief push a item to table
   *
   * @param subName dir name or file name
   * @param subCoverage dir's coverage or file's coverage
   * @param subNumerator
   * @param subDenominator
   * @param numOfFiles in dir (only used if root)
   */
  void pushItemToTable(const std::string& subName,
                     int subCoverage,
                     std::size_t subNumerator,
                     std::size_t subDenominator,
                     std::size_t numOfFiles);

  /**
   * @brief make html string
   */
  std::string str() override;

 private:
  bool mRoot;
  std::string mDirName;
  std::size_t mSizeOfTargetFiles;
  unsigned int mCoverage;
  std::size_t mNumerator;
  std::size_t mDenominator;
  std::string mTableItem;

  std::string indexRootTitle =
      R"(        <h3>Proejct Summary</h3>)";

  std::string indexSubTitle =
      R"(        <h2>Directory Summary</h2>
        <h3>{dir_name}</h3>)";

  std::string indexRootTableCol =
      R"(                    <th>Name</th>
                    <th>Number of Files</th>
                    <th>Mutation Coverage</th>)";

  std::string indexSubTableCol =
      R"(                    <th>Name</th>
                    <th>Mutation Coverage</th>)";
  std::string indexRootTableItem =
      R"(                <tr>
                    <td>
                        <a href="{sub_path}">{sub_name}</a>
                    </td>
                    <td>{num_of_files}</td>
                    <td>
                        <div class="coverage_percentage">{sub_cov}% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-{sub_cov}">
                                <div class="coverage_legend">{sub_numerator}/{sub_denominator}</div>
                            </div>
                        </div>
                    </td>
                </tr>
)";

  std::string indexSubTableItem =
      R"(                <tr>
                    <td>
                        <a href="{sub_path}">{sub_name}</a>
                    </td>
                    <td>
                        <div class="coverage_percentage">{sub_cov}% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-{sub_cov}">
                                <div class="coverage_legend">{sub_numerator}/{sub_denominator}</div>
                            </div>
                        </div>
                    </td>
                </tr>
)";

  std::string indexSkeleton =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="{style_path}style.css"/>
    </head>
    <body>
        <h1>Sentinel Mutation Coverage Report</h1>
{index_title}
        <table>
            <thead>
                <tr>
                    <th>Number of Files</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>{size_of_target_files}</td>
                    <td>{cov}% <div class="coverage_bar"><div class="coverage_complete width-{cov}"><div class="coverage_legend">{numerator}/{denominator}</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <h3>Breakdown by {Directory_or_File}</h3>
        <table>
            <thead>
                <tr>
{table_col}
                </tr>
            </thead>
            <tbody>
{table_item}            </tbody>
        </table>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_INDEXHTMLGENERATOR_HPP_
