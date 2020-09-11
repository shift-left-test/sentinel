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

#include <gtest/gtest.h>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/filesystem.hpp"
#include "sentinel/XMLReport.hpp"


namespace sentinel {

class XMLReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = util::filesystem::tempDirectory("fixture");
    OUT_DIR = util::filesystem::tempDirectory(util::filesystem::join(BASE,
        "OUT_DIR"));

    MUT_RESULT_DIR = util::filesystem::tempDirectory(
        util::filesystem::join(BASE, "MUT_RESLUT_DIR"));
  }

  void TearDown() override {
    util::filesystem::removeDirectories(BASE);
  }

  std::string BASE;
  std::string OUT_DIR;
  std::string MUT_RESULT_DIR;
  std::string TARGET_FULL_PATH = util::filesystem::getAbsolutePath(
      "input/sample1/sample1.cpp");
  std::string EXPECT_MUT_XML_CONTENT = ""
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<mutations>\n"
    "    <mutation detected=\"false\">\n"
    "        <sourceFile>sample1.cpp</sourceFile>\n"
    "        <sourceFilePath>"+TARGET_FULL_PATH+"</sourceFilePath>\n"
    "        <mutatedClass></mutatedClass>\n"
    "        <mutatedMethod></mutatedMethod>\n"
    "        <methodDescription></methodDescription>\n"
    "        <lineNumber>4</lineNumber>\n"
    "        <mutator>AOR</mutator>\n"
    "        <killingTest>testAdd</killingTest>\n"
    "    </mutation>\n"
    "    <mutation detected=\"true\">\n"
    "        <sourceFile>sample1.cpp</sourceFile>\n"
    "        <sourceFilePath>"+TARGET_FULL_PATH+"</sourceFilePath>\n"
    "        <mutatedClass></mutatedClass>\n"
    "        <mutatedMethod></mutatedMethod>\n"
    "        <methodDescription></methodDescription>\n"
    "        <lineNumber>1</lineNumber>\n"
    "        <mutator>BOR</mutator>\n"
    "        <killingTest>testAddBit</killingTest>\n"
    "    </mutation>\n</mutations>\n";
};

TEST_F(XMLReportTest, testMakeXMLReport) {
  Mutable M1("AOR", "input/sample1/sample1.cpp", 4, 5, 6, 7, "+");
  MutationResult MR1(M1, "testAdd", false, 0);
  MR1.saveToFile(MUT_RESULT_DIR);

  Mutable M2("BOR", "input/sample1/sample1.cpp", 1, 2, 3, 4, "|");
  MutationResult MR2(M2, "testAddBit", true, 1);
  MR2.saveToFile(MUT_RESULT_DIR);

  XMLReport xmlreport(MUT_RESULT_DIR);

  xmlreport.save(OUT_DIR);
  auto mutationXMLPath = util::filesystem::findFilesInDirUsingRgx(OUT_DIR,
      std::regex(".*mutations.xml"));
  EXPECT_EQ(1, mutationXMLPath.size());

  std::ifstream t(mutationXMLPath[0]);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string mutationXMLContent = buffer.str();
  EXPECT_EQ(mutationXMLContent, EXPECT_MUT_XML_CONTENT);
}

}  // namespace sentinel
