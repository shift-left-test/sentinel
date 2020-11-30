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
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/XMLReport.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class XMLReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE =
        fs::temp_directory_path() / "SENTINEL_XMLREPORTTEST_TMP_DIR";
    fs::remove_all(BASE);

    OUT_DIR = BASE / "OUT_DIR";
    fs::create_directories(OUT_DIR);

    MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR";
    fs::create_directories(MUT_RESULT_DIR);

    SOURCE_DIR = BASE / "SOURCE_DIR";
    auto NESTED_SOURCE_DIR = SOURCE_DIR / "NESTED_DIR";
    fs::create_directories(NESTED_SOURCE_DIR);

    TARGET_FULL_PATH = SOURCE_DIR / "file1.cpp";
    std::string TARGET_NAME = TARGET_FULL_PATH.filename();
    std::ofstream(TARGET_FULL_PATH).close();

    TARGET_FULL_PATH2 = NESTED_SOURCE_DIR / "file2.cpp";
    std::string TARGET_NAME2 = TARGET_FULL_PATH2.filename();
    std::ofstream(TARGET_FULL_PATH2).close();

    EXPECT_MUT_XML_CONTENT = fmt::format(EXPECT_MUT_XML_CONTENT,
        TARGET_NAME, TARGET_FULL_PATH.filename().string(),
        TARGET_NAME2, NESTED_SOURCE_DIR.filename().string() + "/" +
        TARGET_FULL_PATH2.filename().string());
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  fs::path BASE;
  fs::path OUT_DIR;
  fs::path MUT_RESULT_DIR;
  fs::path SOURCE_DIR;
  fs::path TARGET_FULL_PATH;
  fs::path TARGET_FULL_PATH2;
  std::string EXPECT_MUT_XML_CONTENT = ""
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<mutations>\n"
      "    <mutation detected=\"false\">\n"
      "        <sourceFile>{0}</sourceFile>\n"
      "        <sourceFilePath>{1}</sourceFilePath>\n"
      "        <mutatedClass></mutatedClass>\n"
      "        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>\n"
      "        <lineNumber>4</lineNumber>\n"
      "        <mutator>AOR</mutator>\n"
      "        <killingTest></killingTest>\n"
      "    </mutation>\n"
      "    <mutation detected=\"true\">\n"
      "        <sourceFile>{2}</sourceFile>\n"
      "        <sourceFilePath>{3}</sourceFilePath>\n"
      "        <mutatedClass></mutatedClass>\n"
      "        <mutatedMethod>sumOfEvenPositiveNumber</mutatedMethod>\n"
      "        <lineNumber>1</lineNumber>\n"
      "        <mutator>BOR</mutator>\n"
      "        <killingTest>testAddBit</killingTest>\n"
      "    </mutation>\n</mutations>\n";
};

TEST_F(XMLReportTest, testMakeXMLReport) {
  MutationResults MRs;
  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             4, 5, 6, 7, "+");
  MRs.emplace_back(M1, "", "", MutationState::ALIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             1, 2, 3, 4, "|");
  MRs.emplace_back(M2, "testAddBit", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             1, 2, 3, 4, "|");
  MRs.emplace_back(M3, "testAddBit", "", MutationState::RUNTIME_ERROR);

  Mutant M4("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             1, 2, 3, 4, "|");
  MRs.emplace_back(M4, "testAddBit", "", MutationState::BUILD_FAILURE);

  auto MRPath = MUT_RESULT_DIR / "MutationResult1";
  MRs.save(MRPath);

  XMLReport xmlreport(MRPath, SOURCE_DIR);

  xmlreport.save(OUT_DIR);

  auto outXMLPath = OUT_DIR / "mutations.xml";
  EXPECT_TRUE(fs::exists(outXMLPath));

  std::ifstream t(outXMLPath);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string mutationXMLContent = buffer.str();
  t.close();
  EXPECT_EQ(mutationXMLContent, EXPECT_MUT_XML_CONTENT);
}

TEST_F(XMLReportTest, testSaveFailWhenInvalidDirGiven) {
  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             4, 5, 6, 7, "+");
  MutationResult MR1(M1, "", "", MutationState::ALIVED);

  MutationResults MRs;
  MRs.push_back(MR1);

  XMLReport xmlreport(MRs, SOURCE_DIR);

  EXPECT_THROW(xmlreport.save(TARGET_FULL_PATH), InvalidArgumentException);
  EXPECT_NO_THROW(xmlreport.save("unknown"));
  ASSERT_TRUE(fs::exists("unknown"));
  fs::remove_all("unknown");
}

TEST_F(XMLReportTest, testMakeXMLReportWhenEmptyMutationResult) {
  MutationResults MRs;
  XMLReport xmlreport(MRs, SOURCE_DIR);
  auto OUT_DIR = BASE / "OUT_DIR_EMPTYMUTATIONRESULT";
  xmlreport.save(OUT_DIR);
  EXPECT_FALSE(fs::exists(OUT_DIR));
  EXPECT_FALSE(fs::exists(OUT_DIR / "mutations.xml"));
}

}  // namespace sentinel
