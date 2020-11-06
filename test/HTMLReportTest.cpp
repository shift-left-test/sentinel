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

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/string.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class HTMLReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = fs::temp_directory_path() / "SENTINEL_HTMLREPORTTEST_TMP_DIR";
    fs::remove_all(BASE);

    SOURCE_DIR = BASE / "SOURCE_DIR";
    fs::create_directories(SOURCE_DIR);
    NESTED_SOURCE_DIR = SOURCE_DIR / "NESTED_DIR1/NESTED_DIR";
    fs::create_directories(NESTED_SOURCE_DIR);
    NESTED_SOURCE_DIR2 = SOURCE_DIR / "NESTED_DIR2";
    fs::create_directories(NESTED_SOURCE_DIR2);


    TARGET_FULL_PATH =
        NESTED_SOURCE_DIR / "target1_veryVeryVeryVeryVerylongFilePath.cpp";
    writeFile(TARGET_FULL_PATH, TARGET_CONTENT);
    TARGET_FULL_PATH2 = NESTED_SOURCE_DIR2 / "target2.cpp";
    writeFile(TARGET_FULL_PATH2, TARGET_CONTENT2);
    TARGET_FULL_PATH3 = NESTED_SOURCE_DIR2 / "target3.cpp";
    writeFile(TARGET_FULL_PATH3, TARGET_CONTENT3);
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  void readFileAndCompareExpected(const std::string& path,
      const std::string& expectedContents) {
    EXPECT_TRUE(fs::exists(path));

    std::ifstream t(path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string actualContents = buffer.str();
    t.close();
    EXPECT_EQ(expectedContents, actualContents);
  }

  void writeFile(const std::string& path, const std::string& contents) {
    std::ofstream t(path);
    t << contents;
    t.close();
  }

  fs::path BASE;
  fs::path SOURCE_DIR;
  fs::path NESTED_SOURCE_DIR;
  fs::path NESTED_SOURCE_DIR2;
  fs::path TARGET_FULL_PATH;
  std::string TARGET_CONTENT =
      R"(int add(int a, int b) {
  return a + c;
})";
  fs::path TARGET_FULL_PATH2;
  std::string TARGET_CONTENT2 =
      R"(int bitwiseOR(int a, int b) {
  return a | c;
})";
  fs::path TARGET_FULL_PATH3;
  std::string TARGET_CONTENT3 =
      R"(//a & b
int bitwiseAND(int a, int b){
  return a & b;
}

// a - b
int minus(int a, int b){
  return a - b;
})";
  std::string ORI_ROOT_INDEX_HTML_CONTENTS =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="style.css"/>
    </head>
    <body>
        <h1>Sentinel Mutation Coverage Report</h1>
        <h3>Proejct Summary</h3>
        <table>
            <thead>
                <tr>
                    <th>Number of Files</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>3</td>
                    <td>50% <div class="coverage_bar"><div class="coverage_complete width-50"><div class="coverage_legend">2/4</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <h3>Breakdown by Directory</h3>
        <table>
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Number of Files</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>
                        <a href="./{0}/index.html">{0}</a>
                    </td>
                    <td>1</td>
                    <td>
                        <div class="coverage_percentage">0% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-0">
                                <div class="coverage_legend">0/1</div>
                            </div>
                        </div>
                    </td>
                </tr>
                <tr>
                    <td>
                        <a href="./{1}/index.html">{1}</a>
                    </td>
                    <td>2</td>
                    <td>
                        <div class="coverage_percentage">66% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-66">
                                <div class="coverage_legend">2/3</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
  std::string ORI_NESTED1_INDEX_HTML_CONTENTS =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="../style.css"/>
    </head>
    <body>
        <h1>Sentinel Mutation Coverage Report</h1>
        <h2>Directory Summary</h2>
        <h3>{0}</h3>
        <table>
            <thead>
                <tr>
                    <th>Number of Files</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>1</td>
                    <td>0% <div class="coverage_bar"><div class="coverage_complete width-0"><div class="coverage_legend">0/1</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <h3>Breakdown by File</h3>
        <table>
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>
                        <a href="./{1}.html">{1}</a>
                    </td>
                    <td>
                        <div class="coverage_percentage">0% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-0">
                                <div class="coverage_legend">0/1</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
  std::string ORI_NESTED2_INDEX_HTML_CONTENTS =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="../style.css"/>
    </head>
    <body>
        <h1>Sentinel Mutation Coverage Report</h1>
        <h2>Directory Summary</h2>
        <h3>{0}</h3>
        <table>
            <thead>
                <tr>
                    <th>Number of Files</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>2</td>
                    <td>66% <div class="coverage_bar"><div class="coverage_complete width-66"><div class="coverage_legend">2/3</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <h3>Breakdown by File</h3>
        <table>
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Mutation Coverage</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>
                        <a href="./{1}.html">{1}</a>
                    </td>
                    <td>
                        <div class="coverage_percentage">100% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-100">
                                <div class="coverage_legend">1/1</div>
                            </div>
                        </div>
                    </td>
                </tr>
                <tr>
                    <td>
                        <a href="./{2}.html">{2}</a>
                    </td>
                    <td>
                        <div class="coverage_percentage">50% </div>
                        <div class="coverage_bar">
                            <div class="coverage_complete width-50">
                                <div class="coverage_legend">1/2</div>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
  std::string ORI_TARGET1_HTML_CONTENTS =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="../style.css"/>
    </head>
    <body>
        <h1>{0}</h1>
        <table class="src">
            <tr>
                <td class="na">1<a name="sentinel.report.html.SourceFile@{0}_1"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_1"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>int add(int a, int b) {</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">2<a name="sentinel.report.html.SourceFile@{0}_2"/></td>
                <td class="survived">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_2">1</a>
                        <span>
                            <b>1. AOR -&gt; NO_COVERAGE</b>
                            <br/>
                        </span>
                    </span>
                </td>
                <td class="survived">
                    <span class="survived">
                        <pre>  return a + c;</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">3<a name="sentinel.report.html.SourceFile@{0}_3"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_3"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>}</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td/>
                <td/>
                <td>
                    <h2>Mutations</h2>
                </td>
            </tr>
            <tr>
                <td>
                    <a href="#sentinel.report.html.SourceFile@{0}_2">2</a>
                </td>
                <td/>
                <td>
                    <a name="group.sentinel.report.html.SourceFile@{0}_2"/>
                    <a class="NO_COVERAGE">
                        <span class="pop">1.<span><b>1 </b>
                                <br/>
                                <b>Location: line num 2</b>
                                <br/>
                                <b>Killed by : none</b>
                            </span>
                        </span>
                        <span>AOR -&gt; NO_COVERAGE</span>
                    </a>
                </td>
            </tr>
        </table>
        <h2>Active mutators</h2>
        <ul>
            <li class="mutator">AOR</li>
        </ul>
        <h2>Tests examined</h2>
        <ul/>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
  std::string ORI_TARGET2_HTML_CONTENTS =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="../style.css"/>
    </head>
    <body>
        <h1>{0}</h1>
        <table class="src">
            <tr>
                <td class="na">1<a name="sentinel.report.html.SourceFile@{0}_1"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_1"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>int bitwiseOR(int a, int b) {</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">2<a name="sentinel.report.html.SourceFile@{0}_2"/></td>
                <td class="killed">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_2">1</a>
                        <span>
                            <b>1. BOR -&gt; KILLED</b>
                            <br/>
                        </span>
                    </span>
                </td>
                <td class="killed">
                    <span class="killed">
                        <pre>  return a | c;</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">3<a name="sentinel.report.html.SourceFile@{0}_3"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_3"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>}</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td/>
                <td/>
                <td>
                    <h2>Mutations</h2>
                </td>
            </tr>
            <tr>
                <td>
                    <a href="#sentinel.report.html.SourceFile@{0}_2">2</a>
                </td>
                <td/>
                <td>
                    <a name="group.sentinel.report.html.SourceFile@{0}_2"/>
                    <a class="KILLED">
                        <span class="pop">1.<span><b>1 </b>
                                <br/>
                                <b>Location: line num 2</b>
                                <br/>
                                <b>Killed by : testBitwiseOR</b>
                            </span>
                        </span>
                        <span>BOR -&gt; KILLED</span>
                    </a>
                </td>
            </tr>
        </table>
        <h2>Active mutators</h2>
        <ul>
            <li class="mutator">BOR</li>
        </ul>
        <h2>Tests examined</h2>
        <ul>
            <li>testBitwiseOR</li>
        </ul>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
  std::string ORI_TARGET3_HTML_CONTENTS =
      R"(<!DOCTYPE html>
<html>
    <head>
        <link rel="stylesheet" type="text/css" href="../style.css"/>
    </head>
    <body>
        <h1>{0}</h1>
        <table class="src">
            <tr>
                <td class="na">1<a name="sentinel.report.html.SourceFile@{0}_1"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_1"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>//a & b</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">2<a name="sentinel.report.html.SourceFile@{0}_2"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_2"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>int bitwiseAND(int a, int b){</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">3<a name="sentinel.report.html.SourceFile@{0}_3"/></td>
                <td class="killed">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_3">1</a>
                        <span>
                            <b>1. BOR -&gt; KILLED</b>
                            <br/>
                        </span>
                    </span>
                </td>
                <td class="killed">
                    <span class="killed">
                        <pre>  return a & b;</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">4<a name="sentinel.report.html.SourceFile@{0}_4"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_4"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>}</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">5<a name="sentinel.report.html.SourceFile@{0}_5"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_5"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre></pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">6<a name="sentinel.report.html.SourceFile@{0}_6"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_6"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>// a - b</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">7<a name="sentinel.report.html.SourceFile@{0}_7"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_7"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>int minus(int a, int b){</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">8<a name="sentinel.report.html.SourceFile@{0}_8"/></td>
                <td class="survived">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_8">1</a>
                        <span>
                            <b>1. AOR -&gt; NO_COVERAGE</b>
                            <br/>
                        </span>
                    </span>
                </td>
                <td class="survived">
                    <span class="survived">
                        <pre>  return a - b;</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td class="na">9<a name="sentinel.report.html.SourceFile@{0}_9"/></td>
                <td class="">
                    <span class="pop">
                        <a href="#group.sentinel.report.html.SourceFile@{0}_9"></a>
                        <span>
                        </span>
                    </span>
                </td>
                <td class="">
                    <span class="">
                        <pre>}</pre>
                    </span>
                </td>
            </tr>
            <tr>
                <td/>
                <td/>
                <td>
                    <h2>Mutations</h2>
                </td>
            </tr>
            <tr>
                <td>
                    <a href="#sentinel.report.html.SourceFile@{0}_3">3</a>
                </td>
                <td/>
                <td>
                    <a name="group.sentinel.report.html.SourceFile@{0}_3"/>
                    <a class="KILLED">
                        <span class="pop">1.<span><b>1 </b>
                                <br/>
                                <b>Location: line num 3</b>
                                <br/>
                                <b>Killed by : testBitwiseAND, testBitwiseOP</b>
                            </span>
                        </span>
                        <span>BOR -&gt; KILLED</span>
                    </a>
                </td>
            </tr>
            <tr>
                <td>
                    <a href="#sentinel.report.html.SourceFile@{0}_8">8</a>
                </td>
                <td/>
                <td>
                    <a name="group.sentinel.report.html.SourceFile@{0}_8"/>
                    <a class="NO_COVERAGE">
                        <span class="pop">1.<span><b>1 </b>
                                <br/>
                                <b>Location: line num 8</b>
                                <br/>
                                <b>Killed by : none</b>
                            </span>
                        </span>
                        <span>AOR -&gt; NO_COVERAGE</span>
                    </a>
                </td>
            </tr>
        </table>
        <h2>Active mutators</h2>
        <ul>
            <li class="mutator">AOR</li>
            <li class="mutator">BOR</li>
        </ul>
        <h2>Tests examined</h2>
        <ul>
            <li>testBitwiseAND</li>
            <li>testBitwiseOP</li>
        </ul>
        <hr/>
        <h5>Report generated by <a href="mod.lge.com/hub/yocto/addons/sentinel">Sentinel</a>
        </h5>
    </body>
</html>
)";
};

TEST_F(HTMLReportTest, testMakeHTMLReport) {
  auto OUT_DIR = BASE / "OUT_DIR_MAKEHTMLREPORT1";
  fs::create_directories(OUT_DIR);

  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR";
  fs::create_directories(MUT_RESULT_DIR);

  std::string nestedSourceDir =
    NESTED_SOURCE_DIR.parent_path().filename().string() + "." +
    NESTED_SOURCE_DIR.filename().string();

  std::string ROOT_INDEX_HTML_CONTENTS = fmt::format(
      ORI_ROOT_INDEX_HTML_CONTENTS, nestedSourceDir,
      NESTED_SOURCE_DIR2.filename().string());
  std::string NESTED1_INDEX_HTML_CONTENTS = fmt::format(
      ORI_NESTED1_INDEX_HTML_CONTENTS, nestedSourceDir,
      TARGET_FULL_PATH.filename().string());
  std::string NESTED2_INDEX_HTML_CONTENTS = fmt::format(
      ORI_NESTED2_INDEX_HTML_CONTENTS, NESTED_SOURCE_DIR2.filename().string(),
      TARGET_FULL_PATH2.filename().string(),
      TARGET_FULL_PATH3.filename().string());

  std::string TARGET1_HTML_CONTENTS =
      string::replaceAll(ORI_TARGET1_HTML_CONTENTS,
      "{0}", TARGET_FULL_PATH.filename());
  std::string TARGET2_HTML_CONTENTS =
      string::replaceAll(ORI_TARGET2_HTML_CONTENTS,
      "{0}", TARGET_FULL_PATH2.filename());
  std::string TARGET3_HTML_CONTENTS =
      string::replaceAll(ORI_TARGET3_HTML_CONTENTS,
      "{0}", TARGET_FULL_PATH3.filename());

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::ALIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "",
                     MutationState::KILLED);

  Mutant M4("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M4, "", "", MutationState::ALIVED);

  Mutant M5("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::BUILD_FAILURE);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  HTMLReport htmlreport(MRPath, SOURCE_DIR, false);

  htmlreport.save(OUT_DIR);

  readFileAndCompareExpected(OUT_DIR / "index.html",
      ROOT_INDEX_HTML_CONTENTS);
  readFileAndCompareExpected(
      OUT_DIR / nestedSourceDir / "index.html",
      NESTED1_INDEX_HTML_CONTENTS);
  readFileAndCompareExpected(
      OUT_DIR / NESTED_SOURCE_DIR2.filename() / "index.html",
      NESTED2_INDEX_HTML_CONTENTS);

  readFileAndCompareExpected(OUT_DIR / nestedSourceDir /
      (TARGET_FULL_PATH.filename().string() + ".html"), TARGET1_HTML_CONTENTS);
  readFileAndCompareExpected(OUT_DIR / NESTED_SOURCE_DIR2.filename() /
      (TARGET_FULL_PATH2.filename().string() + ".html"), TARGET2_HTML_CONTENTS);
  readFileAndCompareExpected(OUT_DIR / NESTED_SOURCE_DIR2.filename() /
      (TARGET_FULL_PATH3.filename().string() + ".html"), TARGET3_HTML_CONTENTS);

  auto MRPath2 = MUT_RESULT_DIR / "MutationResult2";
  Mutant M6("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M6, "", "", MutationState::RUNTIME_ERROR);
  MRs.save(MRPath2);

  auto OUT_DIR2 = BASE / "OUT_DIR_MAKEHTMLREPORT2";
  fs::create_directories(OUT_DIR2);

  HTMLReport htmlreport2(MRPath2, SOURCE_DIR, true);

  htmlreport2.save(OUT_DIR2);

  readFileAndCompareExpected(OUT_DIR2 / "index.html",
      ROOT_INDEX_HTML_CONTENTS);
  readFileAndCompareExpected(
      OUT_DIR2 / nestedSourceDir / "index.html",
      NESTED1_INDEX_HTML_CONTENTS);
  readFileAndCompareExpected(
      OUT_DIR2 / NESTED_SOURCE_DIR2.filename() / "index.html",
      NESTED2_INDEX_HTML_CONTENTS);

  readFileAndCompareExpected(OUT_DIR2 / nestedSourceDir /
      (TARGET_FULL_PATH.filename().string() + ".html"), TARGET1_HTML_CONTENTS);
  readFileAndCompareExpected(OUT_DIR2 / NESTED_SOURCE_DIR2.filename() /
      (TARGET_FULL_PATH2.filename().string() + ".html"), TARGET2_HTML_CONTENTS);
  readFileAndCompareExpected(OUT_DIR2 / NESTED_SOURCE_DIR2.filename() /
      (TARGET_FULL_PATH3.filename().string() + ".html"), TARGET3_HTML_CONTENTS);
}

TEST_F(HTMLReportTest, testConstructorFailWhenInvalidPathGiven) {
  EXPECT_THROW(HTMLReport htmlreport("unknown", "unknown"),
               InvalidArgumentException);
}

TEST_F(HTMLReportTest, testSaveFailWhenInvalidDirPathGiven) {
  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::ALIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HTMLReport htmlreport(MRs, SOURCE_DIR);

  EXPECT_THROW(htmlreport.save(TARGET_FULL_PATH), InvalidArgumentException);
  EXPECT_NO_THROW(htmlreport.save("unknown"));
  ASSERT_TRUE(fs::exists("unknown"));
  fs::remove_all("unknown");

  std::string nestedSourceDir =
    NESTED_SOURCE_DIR.parent_path().filename().string() + "." +
    NESTED_SOURCE_DIR.filename().string();

  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDDIRPATHGIVEN";
  fs::create_directories(OUT_DIR);
  auto ERR_FILE = OUT_DIR / nestedSourceDir;
  std::ofstream(ERR_FILE).close();
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);
}

TEST_F(HTMLReportTest, testSaveFailWhenInvalidSourcePath) {
  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDSOURCEPATH";
  fs::create_directories(OUT_DIR);
  auto tmpPath = TARGET_FULL_PATH;
  tmpPath.concat("_tmpPath");
  fs::copy(TARGET_FULL_PATH, tmpPath);
  Mutant M1("AOR", tmpPath, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::ALIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HTMLReport htmlreport(MRs, SOURCE_DIR);
  fs::remove(tmpPath);
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);
}

TEST_F(HTMLReportTest, testSaveFailWhenInvalidLineNumber) {
  auto OUT_DIR = BASE / "OUT_DIR_SAVEFAILWHENINVALIDLINENUMBER";
  fs::create_directories(OUT_DIR);
  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             0, 12, 2, 13, "+");
  MutationResult MR1(M1, "", "", MutationState::ALIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  HTMLReport htmlreport(MRs, SOURCE_DIR);
  EXPECT_THROW(htmlreport.save(OUT_DIR), InvalidArgumentException);

  auto OUT_DIR2 = BASE / "OUT_DIR_SAVEFAILWHENINVALIDLINENUMBER2";
  fs::create_directories(OUT_DIR2);
  Mutant M2("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             1000, 12, 1000, 13, "+");
  MutationResult MR2(M2, "", "", MutationState::ALIVED);

  MutationResults MRs2;
  MRs2.push_back(MR2);

  HTMLReport htmlreport2(MRs2, SOURCE_DIR);
  EXPECT_THROW(htmlreport2.save(OUT_DIR2), InvalidArgumentException);
}


}  // namespace sentinel
