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
#include <string>
#include "sentinel/exceptions/XMLException.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

class ResultTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = os::tempDirectory("fixture");
    ORI_DIR = os::tempDirectory(
        os::path::join(BASE, "ori_dir"));
    MAKE_RESULT_XML(ORI_DIR, TC1);
    MAKE_RESULT_XML(ORI_DIR, TC2);
  }

  void TearDown() override {
    os::removeDirectories(BASE);
  }

  void MAKE_RESULT_XML(const std::string& dirPath,
      const std::string& fileContent) {
    std::string tmp = os::tempFilename(
        os::path::join(dirPath, "pre"), ".xml");
    std::ofstream tmpfile;
    tmpfile.open(tmp.c_str());
    tmpfile << fileContent.c_str();
    tmpfile.close();
  }

  std::string BASE;
  std::string ORI_DIR;
  std::string TC1 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<testsuites tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"0\""
    " time=\"*\"timestamp=\"*\" name=\"AllTests\">\n"
    "\t<testsuite name=\"C1\" tests=\"1\" failures=\"0\" skipped=\"0\""
    " disabled=\"0\" errors=\"0\" time=\"*\" timestamp=\"*\">\n"
    "\t\t<testcase name=\"TC1\" status=\"run\" result=\"completed\""
    " time=\"*\" timestamp=\"*\" classname=\"C1\" />\n"
    "\t</testsuite>\n"
    "</testsuites>\n";
  std::string TC2 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<testsuites tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"0\""
    " time=\"*\"timestamp=\"*\" name=\"AllTests\">\n"
    "\t<testsuite name=\"C2\" tests=\"1\" failures=\"0\" skipped=\"0\""
    " disabled=\"0\" errors=\"0\" time=\"*\" timestamp=\"*\">\n"
    "\t\t<testcase name=\"TC2\" status=\"run\" result=\"completed\""
    " time=\"*\" timestamp=\"*\" classname=\"C2\" />\n"
    "\t</testsuite>\n"
    "</testsuites>\n";
  std::string TC2_FAIL = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<testsuites tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"0\""
    " time=\"*\"timestamp=\"*\" name=\"AllTests\">\n"
    "\t<testsuite name=\"C2\" tests=\"1\" failures=\"0\" skipped=\"0\""
    " disabled=\"0\" errors=\"0\" time=\"*\" timestamp=\"*\">\n"
    "\t\t<testcase name=\"TC2\" status=\"run\" result=\"completed\""
    " time=\"*\" timestamp=\"*\" classname=\"C2\">\n"
    "\t\t\t<failure message=\"fail message\" type=\"\" />"
    "\t\t</testcase>\n"
    "\t</testsuite>\n"
    "</testsuites>\n";
  std::string TC3 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<testsuites tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"0\""
    " time=\"*\"timestamp=\"*\" name=\"AllTests\">\n"
    "\t<testsuite name=\"C1\" tests=\"1\" failures=\"0\" skipped=\"0\""
    " disabled=\"0\" errors=\"0\" time=\"*\" timestamp=\"*\">\n"
    "\t\t<testcase name=\"TC3\" status=\"run\" result=\"completed\""
    " time=\"*\" timestamp=\"*\" classname=\"C1\" />\n"
    "\t</testsuite>\n"
    "</testsuites>\n";
};

TEST_F(ResultTest, testResultWithAliveMutation) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "");
}

TEST_F(ResultTest, testResultWithKillMutation) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithAliveMutationAddNewTC) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2);
  MAKE_RESULT_XML(MUT_DIR, TC3);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "");
}

TEST_F(ResultTest, testResultWithKillMutationAddNewTC) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);
  MAKE_RESULT_XML(MUT_DIR, TC3);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithKKillMutationErrorTC) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, TC1);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithKKillMutationErrorTCAndAddNewTc) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC3);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithEmptyMutationDir) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C1.TC1, C2.TC2");
}

TEST_F(ResultTest, testResultWithWrongXMLFmt) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, string::replaceAll(TC3, "</testsuites>", ""));
  EXPECT_THROW({
      try {
        Result mut(MUT_DIR);
      }
      catch (const XMLException& e){
        EXPECT_TRUE(string::contains(e.what(), "XML_ERROR_PARSING"));
        throw;
      }
  }, XMLException);
}

TEST_F(ResultTest, testResultWithWrongResultFmt) {
  std::string MUT_DIR = os::tempDirectory(
      os::path::join(BASE, "mut_dir"));
  MAKE_RESULT_XML(MUT_DIR, string::replaceAll(TC3, "testsuites", "tag"));
  EXPECT_THROW({
      try {
        Result mut(MUT_DIR);
      }
      catch (const XMLException& e){
        EXPECT_TRUE(string::contains(e.what(),
            "This file doesn't follow googletest result format"));
        throw;
      }
  }, XMLException);
}

}  // namespace sentinel
