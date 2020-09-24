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
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include "sentinel/HTMLReport.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

class HTMLReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = os::tempDirectory("fixture");
    OUT_DIR = os::tempDirectory(os::path::join(BASE,
        "OUT_DIR"));

    MUT_RESULT_DIR = os::tempDirectory(
        os::path::join(BASE, "MUT_RESLUT_DIR"));

    SOURCE_DIR = os::tempDirectory(
        os::path::join(BASE, "SOURCE_DIR"));

    std::string NESTED_SOURCE_DIR = os::tempDirectory(
        os::path::join(SOURCE_DIR, "NESTED_DIR1"));

    std::string NESTED_SOURCE_DIR2 = os::tempDirectory(
        os::path::join(SOURCE_DIR, "NESTED_DIR2"));

    TARGET_FULL_PATH = os::tempFilename(
        NESTED_SOURCE_DIR + "/target1", ".cpp");
    writeFile(TARGET_FULL_PATH, TARGET_CONTENT);
    TARGET_FULL_PATH2 = os::tempFilename(
        NESTED_SOURCE_DIR2 + "/target2", ".cpp");
    writeFile(TARGET_FULL_PATH2, TARGET_CONTENT2);
    TARGET_FULL_PATH3 = os::tempFilename(
        NESTED_SOURCE_DIR2 + "/target3", ".cpp");
    writeFile(TARGET_FULL_PATH3, TARGET_CONTENT3);

    std::string NESTED_DIR1_DOT = string::replaceAll(
        os::path::getRelativePath(NESTED_SOURCE_DIR, SOURCE_DIR), "/", ".");
    std::string NESTED_DIR2_DOT = string::replaceAll(
        os::path::getRelativePath(NESTED_SOURCE_DIR2, SOURCE_DIR), "/", ".");


    ROOT_INDEX_HTML_CONTENTS = fmt::format(
        ROOT_INDEX_HTML_CONTENTS, NESTED_DIR1_DOT, NESTED_DIR2_DOT);
    NESTED1_INDEX_HTML_CONTENTS = fmt::format(
        NESTED1_INDEX_HTML_CONTENTS, NESTED_DIR1_DOT,
        os::path::filename(TARGET_FULL_PATH));
    NESTED2_INDEX_HTML_CONTENTS = fmt::format(
        NESTED2_INDEX_HTML_CONTENTS, NESTED_DIR2_DOT,
        os::path::filename(TARGET_FULL_PATH2),
        os::path::filename(TARGET_FULL_PATH3));

    TARGET1_HTML_CONTENTS = string::replaceAll(TARGET1_HTML_CONTENTS,
        "{0}", os::path::filename(TARGET_FULL_PATH));
    TARGET2_HTML_CONTENTS = string::replaceAll(TARGET2_HTML_CONTENTS,
        "{0}", os::path::filename(TARGET_FULL_PATH2));
    TARGET3_HTML_CONTENTS = string::replaceAll(TARGET3_HTML_CONTENTS,
        "{0}", os::path::filename(TARGET_FULL_PATH3));
  }

  void TearDown() override {
    os::removeDirectories(BASE);
  }

  void readFileAndCompareExpected(const std::string& path,
      const std::string& expectedContents) {
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

  std::string BASE;
  std::string OUT_DIR;
  std::string MUT_RESULT_DIR;
  std::string SOURCE_DIR;
  std::string TARGET_FULL_PATH;
  std::string TARGET_CONTENT = ""
      "int add(int a, int b) {\n"
      "  return a + c;\n"
      "}";
  std::string TARGET_FULL_PATH2;
  std::string TARGET_CONTENT2 = ""
      "int bitwiseOR(int a, int b) {\n"
      "  return a | c;\n"
      "}";
  std::string TARGET_FULL_PATH3;
  std::string TARGET_CONTENT3 = ""
      "//a & b\n"
      "int bitwiseAND(int a, int b){\n"
      "  return a & b;\n"
      "}\n"
      "\n"
      "// a - b\n"
      "int minus(int a, int b){\n"
      "  return a - b;\n"
      "}";
  std::string ROOT_INDEX_HTML_CONTENTS = ""
      "<!DOCTYPE html>\n"
      "<html>\n"
      "    <head>\n"
      "        <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"/>\n" // NOLINT
      "    </head>\n"
      "    <body>\n"
      "        <h1>Sentinel Mutation Coverage Report</h1>\n"
      "        <h3>Proejct Summary</h3>\n"
      "        <table>\n"
      "            <thead>\n"
      "                <tr>\n"
      "                    <th>Number of Files</th>\n"
      "                    <th>Mutation Coverage</th>\n"
      "                </tr>\n"
      "            </thead>\n"
      "            <tbody>\n"
      "                <tr>\n"
      "                    <td>3</td>\n"
      "                    <td>50% <div class=\"coverage_bar\"><div class=\"coverage_complete width-50\"><div class=\"coverage_legend\">2/4</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "            </tbody>\n"
      "        </table>\n"
      "        <h3>Breakdown by Directory</h3>\n"
      "        <table>\n"
      "            <thead>\n"
      "                <tr>\n"
      "                    <th>Name</th>\n"
      "                    <th>Number of Files</th>\n"
      "                    <th>Mutation Coverage</th>\n"
      "                </tr>\n"
      "            </thead>\n"
      "            <tbody>\n"
      "                <tr>\n"
      "                    <td>\n"
      "                        <a href=\"./{0}/index.html\">{0}</a>\n"
      "                    </td>\n"
      "                    <td>1</td>\n"
      "                    <td>\n"
      "                        <div class=\"coverage_percentage\">0% </div>\n"
      "                        <div class=\"coverage_bar\">\n"
      "                            <div class=\"coverage_complete width-0\">\n"
      "                                <div class=\"coverage_legend\">0/1</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "                <tr>\n"
      "                    <td>\n"
      "                        <a href=\"./{1}/index.html\">{1}</a>\n"
      "                    </td>\n"
      "                    <td>2</td>\n"
      "                    <td>\n"
      "                        <div class=\"coverage_percentage\">66% </div>\n"
      "                        <div class=\"coverage_bar\">\n"
      "                            <div class=\"coverage_complete width-66\">\n"
      "                                <div class=\"coverage_legend\">2/3</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "            </tbody>\n"
      "        </table>\n"
      "        <hr/>\n"
      "        <h5>Report generated by <a href=\"http://mod.lge.com/hub/yocto/addons/sentinel\">Sentinel</a>\n" // NOLINT
      "        </h5>\n"
      "    </body>\n"
      "</html>\n";
  std::string NESTED1_INDEX_HTML_CONTENTS = ""
      "<!DOCTYPE html>\n"
      "<html>\n"
      "    <head>\n"
      "        <link rel=\"stylesheet\" type=\"text/css\" href=\"../style.css\"/>\n" //NOLINT
      "    </head>\n"
      "    <body>\n"
      "        <h1>Sentinel Mutation Coverage Report</h1>\n"
      "        <h2>Directory Summary</h2>\n"
      "        <h3>{0}</h3>\n"
      "        <table>\n"
      "            <thead>\n"
      "                <tr>\n"
      "                    <th>Number of Files</th>\n"
      "                    <th>Mutation Coverage</th>\n"
      "                </tr>\n"
      "            </thead>\n"
      "            <tbody>\n"
      "                <tr>\n"
      "                    <td>1</td>\n"
      "                    <td>0% <div class=\"coverage_bar\"><div class=\"coverage_complete width-0\"><div class=\"coverage_legend\">0/1</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "            </tbody>\n"
      "        </table>\n"
      "        <h3>Breakdown by File</h3>\n"
      "        <table>\n"
      "            <thead>\n"
      "                <tr>\n"
      "                    <th>Name</th>\n"
      "                    <th>Mutation Coverage</th>\n"
      "                </tr>\n"
      "            </thead>\n"
      "            <tbody>\n"
      "                <tr>\n"
      "                    <td>\n"
      "                        <a href=\"./{1}.html\">{1}</a>\n"
      "                    </td>\n"
      "                    <td>\n"
      "                        <div class=\"coverage_percentage\">0% </div>\n"
      "                        <div class=\"coverage_bar\">\n"
      "                            <div class=\"coverage_complete width-0\">\n"
      "                                <div class=\"coverage_legend\">0/1</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "            </tbody>\n"
      "        </table>\n"
      "        <hr/>\n"
      "        <h5>Report generated by <a href=\"http://mod.lge.com/hub/yocto/addons/sentinel\">Sentinel</a>\n" // NOLINT
      "        </h5>\n"
      "    </body>\n"
      "</html>\n";
  std::string NESTED2_INDEX_HTML_CONTENTS = ""
      "<!DOCTYPE html>\n"
      "<html>\n"
      "    <head>\n"
      "        <link rel=\"stylesheet\" type=\"text/css\" href=\"../style.css\"/>\n" // NOLINT
      "    </head>\n"
      "    <body>\n"
      "        <h1>Sentinel Mutation Coverage Report</h1>\n"
      "        <h2>Directory Summary</h2>\n"
      "        <h3>{0}</h3>\n"
      "        <table>\n"
      "            <thead>\n"
      "                <tr>\n"
      "                    <th>Number of Files</th>\n"
      "                    <th>Mutation Coverage</th>\n"
      "                </tr>\n"
      "            </thead>\n"
      "            <tbody>\n"
      "                <tr>\n"
      "                    <td>2</td>\n"
      "                    <td>66% <div class=\"coverage_bar\"><div class=\"coverage_complete width-66\"><div class=\"coverage_legend\">2/3</div>\n" //NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "            </tbody>\n"
      "        </table>\n"
      "        <h3>Breakdown by File</h3>\n"
      "        <table>\n"
      "            <thead>\n"
      "                <tr>\n"
      "                    <th>Name</th>\n"
      "                    <th>Mutation Coverage</th>\n"
      "                </tr>\n"
      "            </thead>\n"
      "            <tbody>\n"
      "                <tr>\n"
      "                    <td>\n"
      "                        <a href=\"./{1}.html\">{1}</a>\n"
      "                    </td>\n"
      "                    <td>\n"
      "                        <div class=\"coverage_percentage\">100% </div>\n"
      "                        <div class=\"coverage_bar\">\n"
      "                            <div class=\"coverage_complete width-100\">\n" // NOLINT
      "                                <div class=\"coverage_legend\">1/1</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "                <tr>\n"
      "                    <td>\n"
      "                        <a href=\"./{2}.html\">{2}</a>\n"
      "                    </td>\n"
      "                    <td>\n"
      "                        <div class=\"coverage_percentage\">50% </div>\n"
      "                        <div class=\"coverage_bar\">\n"
      "                            <div class=\"coverage_complete width-50\">\n"
      "                                <div class=\"coverage_legend\">1/2</div>\n" // NOLINT
      "                            </div>\n"
      "                        </div>\n"
      "                    </td>\n"
      "                </tr>\n"
      "            </tbody>\n"
      "        </table>\n"
      "        <hr/>\n"
      "        <h5>Report generated by <a href=\"http://mod.lge.com/hub/yocto/addons/sentinel\">Sentinel</a>\n" // NOLINT
      "        </h5>\n"
      "    </body>\n"
      "</html>\n";
  std::string TARGET1_HTML_CONTENTS = ""
      "<!DOCTYPE html>\n"
      "<html>\n"
      "    <head>\n"
      "        <link rel=\"stylesheet\" type=\"text/css\" href=\"../style.css\"/>\n" // NOLINT
      "    </head>\n"
      "    <body>\n"
      "        <h1>{0}</h1>\n"
      "        <table class=\"src\">\n"
      "            <tr>\n"
      "                <td class=\"na\">1<a name=\"sentinel.report.html.SourceFile@{0}_1\"/></td>\n" //NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_1\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>int add(int a, int b) {</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">2<a name=\"sentinel.report.html.SourceFile@{0}_2\"/></td>\n" // NOLINT
      "                <td class=\"survived\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_2\">1</a>\n" // NOLINT
      "                        <span>\n"
      "                            <b>1. AOR -&gt; NO_COVERAGE</b>\n"
      "                            <br/>\n"
      "                        </span>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"survived\">\n"
      "                    <span class=\"survived\">\n"
      "                        <pre>  return a + c;</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">3<a name=\"sentinel.report.html.SourceFile@{0}_3\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_3\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>}</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td/>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <h2>Mutations</h2>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td>\n"
      "                    <a href=\"#sentinel.report.html.SourceFile@{0}_2\">2</a>\n" // NOLINT
      "                </td>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <a name=\"group.sentinel.report.html.SourceFile@{0}_2\"/>\n" // NOLINT
      "                    <a class=\"NO_COVERAGE\">\n"
      "                        <span class=\"pop\">1.<span><b>1 </b>\n"
      "                                <br/>\n"
      "                                <b>Location: line num 2</b>\n"
      "                                <br/>\n"
      "                                <b>Killed by : none</b>\n"
      "                            </span>\n"
      "                        </span>\n"
      "                        <span>AOR -&gt; NO_COVERAGE</span>\n"
      "                    </a>\n"
      "                </td>\n"
      "            </tr>\n"
      "        </table>\n"
      "        <h2>Active mutators</h2>\n"
      "        <ul>\n"
      "            <li class=\"mutator\">AOR</li>\n"
      "        </ul>\n"
      "        <h2>Tests examined</h2>\n"
      "        <ul/>\n"
      "        <hr/>\n"
      "        <h5>Report generated by <a href=\"http://mod.lge.com/hub/yocto/addons/sentinel\">Sentinel</a>\n" // NOLINT
      "        </h5>\n"
      "    </body>\n"
      "</html>\n";
  std::string TARGET2_HTML_CONTENTS = ""
      "<!DOCTYPE html>\n"
      "<html>\n"
      "    <head>\n"
      "        <link rel=\"stylesheet\" type=\"text/css\" href=\"../style.css\"/>\n" // NOLINT
      "    </head>\n"
      "    <body>\n"
      "        <h1>{0}</h1>\n"
      "        <table class=\"src\">\n"
      "            <tr>\n"
      "                <td class=\"na\">1<a name=\"sentinel.report.html.SourceFile@{0}_1\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_1\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>int bitwiseOR(int a, int b) {</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">2<a name=\"sentinel.report.html.SourceFile@{0}_2\"/></td>\n" // NOLINT
      "                <td class=\"killed\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_2\">1</a>\n" // NOLINT
      "                        <span>\n"
      "                            <b>1. BOR -&gt; KILLED</b>\n"
      "                            <br/>\n"
      "                        </span>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"killed\">\n"
      "                    <span class=\"killed\">\n"
      "                        <pre>  return a | c;</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">3<a name=\"sentinel.report.html.SourceFile@{0}_3\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_3\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>}</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td/>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <h2>Mutations</h2>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td>\n"
      "                    <a href=\"#sentinel.report.html.SourceFile@{0}_2\">2</a>\n" // NOLINT
      "                </td>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <a name=\"group.sentinel.report.html.SourceFile@{0}_2\"/>\n" // NOLINT
      "                    <a class=\"KILLED\">\n"
      "                        <span class=\"pop\">1.<span><b>1 </b>\n"
      "                                <br/>\n"
      "                                <b>Location: line num 2</b>\n"
      "                                <br/>\n"
      "                                <b>Killed by : testBitwiseOR</b>\n"
      "                            </span>\n"
      "                        </span>\n"
      "                        <span>BOR -&gt; KILLED</span>\n"
      "                    </a>\n"
      "                </td>\n"
      "            </tr>\n"
      "        </table>\n"
      "        <h2>Active mutators</h2>\n"
      "        <ul>\n"
      "            <li class=\"mutator\">BOR</li>\n"
      "        </ul>\n"
      "        <h2>Tests examined</h2>\n"
      "        <ul>\n"
      "            <li>testBitwiseOR</li>\n"
      "        </ul>\n"
      "        <hr/>\n"
      "        <h5>Report generated by <a href=\"http://mod.lge.com/hub/yocto/addons/sentinel\">Sentinel</a>\n" // NOLINT
      "        </h5>\n"
      "    </body>\n"
      "</html>\n";
  std::string TARGET3_HTML_CONTENTS = ""
      "<!DOCTYPE html>\n"
      "<html>\n"
      "    <head>\n"
      "        <link rel=\"stylesheet\" type=\"text/css\" href=\"../style.css\"/>\n" // NOLINT
      "    </head>\n"
      "    <body>\n"
      "        <h1>{0}</h1>\n"
      "        <table class=\"src\">\n"
      "            <tr>\n"
      "                <td class=\"na\">1<a name=\"sentinel.report.html.SourceFile@{0}_1\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_1\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>//a &amp; b</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">2<a name=\"sentinel.report.html.SourceFile@{0}_2\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_2\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>int bitwiseAND(int a, int b){</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">3<a name=\"sentinel.report.html.SourceFile@{0}_3\"/></td>\n" // NOLINT
      "                <td class=\"killed\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_3\">1</a>\n" // NOLINT
      "                        <span>\n"
      "                            <b>1. BOR -&gt; KILLED</b>\n"
      "                            <br/>\n"
      "                        </span>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"killed\">\n"
      "                    <span class=\"killed\">\n"
      "                        <pre>  return a &amp; b;</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">4<a name=\"sentinel.report.html.SourceFile@{0}_4\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_4\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>}</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">5<a name=\"sentinel.report.html.SourceFile@{0}_5\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_5\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre></pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">6<a name=\"sentinel.report.html.SourceFile@{0}_6\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_6\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>// a - b</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">7<a name=\"sentinel.report.html.SourceFile@{0}_7\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_7\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>int minus(int a, int b){</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">8<a name=\"sentinel.report.html.SourceFile@{0}_8\"/></td>\n" // NOLINT
      "                <td class=\"survived\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_8\">1</a>\n" // NOLINT
      "                        <span>\n"
      "                            <b>1. AOR -&gt; NO_COVERAGE</b>\n"
      "                            <br/>\n"
      "                        </span>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"survived\">\n"
      "                    <span class=\"survived\">\n"
      "                        <pre>  return a - b;</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td class=\"na\">9<a name=\"sentinel.report.html.SourceFile@{0}_9\"/></td>\n" // NOLINT
      "                <td class=\"\">\n"
      "                    <span class=\"pop\">\n"
      "                        <a href=\"#group.sentinel.report.html.SourceFile@{0}_9\"></a>\n" // NOLINT
      "                        <span/>\n"
      "                    </span>\n"
      "                </td>\n"
      "                <td class=\"\">\n"
      "                    <span class=\"\">\n"
      "                        <pre>}</pre>\n"
      "                    </span>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td/>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <h2>Mutations</h2>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td>\n"
      "                    <a href=\"#sentinel.report.html.SourceFile@{0}_3\">3</a>\n" // NOLINT
      "                </td>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <a name=\"group.sentinel.report.html.SourceFile@{0}_3\"/>\n" // NOLINT
      "                    <a class=\"KILLED\">\n"
      "                        <span class=\"pop\">1.<span><b>1 </b>\n"
      "                                <br/>\n"
      "                                <b>Location: line num 3</b>\n"
      "                                <br/>\n"
      "                                <b>Killed by : testBitwiseAND, testBitwiseOP</b>\n" // NOLINT
      "                            </span>\n"
      "                        </span>\n"
      "                        <span>BOR -&gt; KILLED</span>\n"
      "                    </a>\n"
      "                </td>\n"
      "            </tr>\n"
      "            <tr>\n"
      "                <td>\n"
      "                    <a href=\"#sentinel.report.html.SourceFile@{0}_8\">8</a>\n" // NOLINT
      "                </td>\n"
      "                <td/>\n"
      "                <td>\n"
      "                    <a name=\"group.sentinel.report.html.SourceFile@{0}_8\"/>\n" // NOLINT
      "                    <a class=\"NO_COVERAGE\">\n"
      "                        <span class=\"pop\">1.<span><b>1 </b>\n"
      "                                <br/>\n"
      "                                <b>Location: line num 8</b>\n"
      "                                <br/>\n"
      "                                <b>Killed by : none</b>\n"
      "                            </span>\n"
      "                        </span>\n"
      "                        <span>AOR -&gt; NO_COVERAGE</span>\n"
      "                    </a>\n"
      "                </td>\n"
      "            </tr>\n"
      "        </table>\n"
      "        <h2>Active mutators</h2>\n"
      "        <ul>\n"
      "            <li class=\"mutator\">AOR</li>\n"
      "            <li class=\"mutator\">BOR</li>\n"
      "        </ul>\n"
      "        <h2>Tests examined</h2>\n"
      "        <ul>\n"
      "            <li>testBitwiseAND</li>\n"
      "            <li>testBitwiseOP</li>\n"
      "        </ul>\n"
      "        <hr/>\n"
      "        <h5>Report generated by <a href=\"http://mod.lge.com/hub/yocto/addons/sentinel\">Sentinel</a>\n" // NOLINT
      "        </h5>\n"
      "    </body>\n"
      "</html>\n";
};

TEST_F(HTMLReportTest, testMakeHTMLReport) {
  Mutable M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MutationResult MR1(M1, "", false);

  Mutable M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "|");
  MutationResult MR2(M2, "testBitwiseOR", true);

  Mutable M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             3, 12, 3, 13, "&");
  MutationResult MR3(M3, "testBitwiseAND, testBitwiseOP", true);

  Mutable M4("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MutationResult MR4(M4, "", false);

  MutationResults MRs;
  MRs.push_back(MR1);
  MRs.push_back(MR2);
  MRs.push_back(MR3);
  MRs.push_back(MR4);
  auto MRPath = os::path::join(MUT_RESULT_DIR, "MutationResult");
  MRs.save(MRPath);

  HTMLReport htmlreport(MRPath, SOURCE_DIR);

  htmlreport.save(OUT_DIR);

  auto mutationHtmlPath = os::findFilesInDirUsingRgx(OUT_DIR,
      std::regex(".*index\\.html"));
  EXPECT_EQ(3, mutationHtmlPath.size());

  for ( auto const& mp : mutationHtmlPath ) {
    if (string::contains(mp, "NESTED_DIR1")) {
      readFileAndCompareExpected(mp, NESTED1_INDEX_HTML_CONTENTS);
    } else if (string::contains(mp, "NESTED_DIR2")) {
      readFileAndCompareExpected(mp, NESTED2_INDEX_HTML_CONTENTS);
    } else if (!string::contains(mp, "NESTED_DIR") &&
        string::contains(mp, "OUT_DIR")) {
      readFileAndCompareExpected(mp, ROOT_INDEX_HTML_CONTENTS);
    } else {
      FAIL() << "Unexpected index.html file detected : " + mp;
    }
  }

  auto srcHtmlPath = os::findFilesInDirUsingRgx(OUT_DIR,
      std::regex(".*target.*\\.cpp\\.html"));
  EXPECT_EQ(3, srcHtmlPath.size());

  for ( auto const& ms : srcHtmlPath ) {
    if (string::contains(ms, "target1")) {
      readFileAndCompareExpected(ms, TARGET1_HTML_CONTENTS);
    } else if (string::contains(ms, "target2")) {
      readFileAndCompareExpected(ms, TARGET2_HTML_CONTENTS);
    } else if (string::contains(ms, "target3")) {
      readFileAndCompareExpected(ms, TARGET3_HTML_CONTENTS);
    } else {
      FAIL() << "Unexpected target*.cpp.html file detected : " + ms;
    }
  }

  testing::internal::CaptureStdout();
  htmlreport.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out, "Directory: " +
                               os::path::getAbsolutePath(SOURCE_DIR)));
  EXPECT_TRUE(string::contains(out, "      0         1        0%"));
  EXPECT_TRUE(string::contains(out, "      1         1      100%"));
  EXPECT_TRUE(string::contains(out, "      1         2       50%"));
  EXPECT_TRUE(string::contains(out, "      2         4       50%"));
}

}  // namespace sentinel
