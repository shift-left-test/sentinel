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

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_SRCHTMLGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_SRCHTMLGENERATOR_HPP_

#include <stddef.h>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/docGenerator/DOCGenerator.hpp"

namespace sentinel {

/**
 * @brief SrcHTMLGenerator class
 */
class SrcHTMLGenerator : public DOCGenerator {
 public:
  /**
   * @brief Default constructor
   *
   * @param srcName current source file name
   */
  explicit SrcHTMLGenerator(const std::string& srcName);

  /**
   * @brief push a source line
   *
   * @param curLineNum current line number
   * @param curClass killed or not or mixed
   * @param numCurLineMrs current line's number of mutation results
   * @param curCode current line's code
   * @param explain vector of tuple (count, operator, killed or not)
   */
  void pushLine(std::size_t curLineNum,
                const std::string& curClass,
                std::size_t numCurLineMrs,
                const std::string& curCode,
                const std::vector<std::tuple<int, std::string, bool>>& explain);

  /**
   * @brief push a mutation 
   *
   * @param curLineNum current line number
   * @param killed or not
   * @param count
   * @param curKillingTest current killing test
   * @param curOperator current mutation operatior
   */
  void pushMutation(std::size_t curLineNum,
                    bool killed,
                    std::size_t count,
                    const std::string& curKillingTest,
                    const std::string& curOperator);

  /**
   * @brief push a mutation operator 
   *
   * @param mutator
   */
  void pushMutator(const std::string& mutator);

  /**
   * @brief push a killingTest
   *
   * @param killingTest
   */
  void pushKillingTest(const std::string& killingTest);

  /**
   * @brief make html string
   */
  std::string str() override;

 private:
  std::string mSrcName;
  std::string mLines;
  std::string mMutations;
  std::string mMutators;
  std::string mTestList;

  std::string lineContent =
      R"(            <tr>
                <td class="na">{cur_lineNum}<a name="sentinel.report.html.SourceFile@{src_name}_{cur_lineNum}"/></td>
                <td class="{cur_class}">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{src_name}_{cur_lineNum}">{num_cur_line_mrs}</a>
                        <span>
{line_explain}                        </span>
                    </span>
                </td>
                <td class="{cur_class}">
                    <span class="{cur_class}">
                        <pre>{cur_code}</pre>
                    </span>
                </td>
            </tr>
)";

  std::string lineExplainContent =
      R"(                            <b>{count}. {operator} -&gt; {killed_or_not}</b>
                            <br/>
)";

  std::string mutationsContent =
      R"(            <tr>
                <td>
                    <a href="#sentinel.report.html.SourceFile@{src_name}_{cur_lineNum}">{cur_lineNum}</a>
                </td>
                <td/>
                <td>
                    <a name="group.sentinel.report.html.SourceFile@{src_name}_{cur_lineNum}"/>
                    <a class="{killed_or_not}">
                        <span class="pop">{count}.<span><b>{count} </b>
                                <br/>
                                <b>Location: line num {cur_lineNum}</b>
                                <br/>
                                <b>Killed by : {cur_killing_test}</b>
                            </span>
                        </span>
                        <span>{operator} -&gt; {killed_or_not}</span>
                    </a>
                </td>
            </tr>
)";

  std::string mutatorListContent =
      R"(            <li class="mutator">{mutator}</li>
)";

  std::string testListContent =
      R"(            <li>{test_function}</li>
)";

  std::string testListGuardContent =
      R"(        <ul>
{test_list}        </ul>
)";

  std::string testListEmptyContent =
      R"(        <ul/>
)";

  std::string srcHtmlSkeleton =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="../style.css"/>
    </head>
    <body>
        <h1>{src_name}</h1>
        <table class="src">
{lines}            <tr>
                <td/>
                <td/>
                <td>
                    <h2>Mutations</h2>
                </td>
            </tr>
{mutations}        </table>
        <h2>Active mutators</h2>
        <ul>
{mutator_list}        </ul>
        <h2>Tests examined</h2>
{test_list_guard}        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_SRCHTMLGENERATOR_HPP_