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
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/XMLReport.hpp"


namespace sentinel {

class XMLReportTest : public ::testing::Test {
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
        os::path::join(SOURCE_DIR, "NESTED_DIR"));

    TARGET_FULL_PATH = os::tempFilenameWithSuffix(
        SOURCE_DIR + "/", ".cpp");
    std::string TARGET_NAME = os::path::filename(TARGET_FULL_PATH);
    TARGET_FULL_PATH2 = os::tempFilenameWithSuffix(
        NESTED_SOURCE_DIR + "/", ".cpp");
    std::string TARGET_NAME2 = os::path::filename(TARGET_FULL_PATH2);
    EXPECT_MUT_XML_CONTENT = fmt::format(EXPECT_MUT_XML_CONTENT,
        TARGET_NAME, os::path::filename(TARGET_FULL_PATH),
        TARGET_NAME2, os::path::filename(NESTED_SOURCE_DIR) + "/" +
        os::path::filename(TARGET_FULL_PATH2));
  }

  void TearDown() override {
    os::removeDirectories(BASE);
  }
  std::string BASE;
  std::string OUT_DIR;
  std::string MUT_RESULT_DIR;
  std::string SOURCE_DIR;
  std::string TARGET_FULL_PATH;
  std::string TARGET_FULL_PATH2;
  std::string EXPECT_MUT_XML_CONTENT = ""
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<mutations>\n"
      "    <mutation detected=\"false\">\n"
      "        <sourceFile>{0}</sourceFile>\n"
      "        <sourceFilePath>{1}</sourceFilePath>\n"
      "        <mutatedClass></mutatedClass>\n"
      "        <mutatedMethod></mutatedMethod>\n"
      "        <methodDescription></methodDescription>\n"
      "        <lineNumber>4</lineNumber>\n"
      "        <mutator>AOR</mutator>\n"
      "        <killingTest></killingTest>\n"
      "    </mutation>\n"
      "    <mutation detected=\"true\">\n"
      "        <sourceFile>{2}</sourceFile>\n"
      "        <sourceFilePath>{3}</sourceFilePath>\n"
      "        <mutatedClass></mutatedClass>\n"
      "        <mutatedMethod></mutatedMethod>\n"
      "        <methodDescription></methodDescription>\n"
      "        <lineNumber>1</lineNumber>\n"
      "        <mutator>BOR</mutator>\n"
      "        <killingTest>testAddBit</killingTest>\n"
      "    </mutation>\n</mutations>\n";
};

TEST_F(XMLReportTest, testMakeXMLReport) {
  Mutable M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             4, 5, 6, 7, "+");
  MutationResult MR1(M1, "", false, 0);
  MR1.saveToFile(MUT_RESULT_DIR);

  Mutable M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             1, 2, 3, 4, "|");
  MutationResult MR2(M2, "testAddBit", true, 1);
  MR2.saveToFile(MUT_RESULT_DIR);

  XMLReport xmlreport(MUT_RESULT_DIR, SOURCE_DIR);

  xmlreport.save(OUT_DIR);
  auto mutationXMLPath = os::findFilesInDirUsingRgx(OUT_DIR,
      std::regex(".*mutations.xml"));
  EXPECT_EQ(1, mutationXMLPath.size());

  std::ifstream t(mutationXMLPath[0]);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string mutationXMLContent = buffer.str();
  t.close();
  EXPECT_EQ(mutationXMLContent, EXPECT_MUT_XML_CONTENT);
}

}  // namespace sentinel
