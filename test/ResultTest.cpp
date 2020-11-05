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
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/string.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class ResultTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = fs::temp_directory_path() / "SENTINEL_RESULTTEST_TMP_DIR";
    fs::remove_all(BASE);
    fs::create_directories(BASE);

    ORI_DIR = BASE / "ori_dir";
    fs::create_directories(ORI_DIR);
    MAKE_RESULT_XML(ORI_DIR, TC1);
    MAKE_RESULT_XML(ORI_DIR, TC2);
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  void MAKE_RESULT_XML(const fs::path& dirPath,
      const std::string& fileContent) {
    static int inc = 0;
    std::ofstream tmpfile;
    tmpfile.open(dirPath / fmt::format("tmp{0}.xml", inc++));
    tmpfile << fileContent.c_str();
    tmpfile.close();
  }

  void MAKE_AND_TEST_WRONG_RESULT_XML(const std::string& givenXMLContents,
      const std::string& targetTag, const std::string& ignoreTag = "") {
    static int inc = 0;
    std::string XMLContents = givenXMLContents;
    auto MUT_DIR = BASE /
        fmt::format("mut_dir_make_and_test_wrong_result_xml_{0}", inc++);
    fs::create_directories(MUT_DIR);
    std::string tmpTag = "1A2B3C4D5F";

    if (!ignoreTag.empty()) {
      XMLContents = string::replaceAll(XMLContents, ignoreTag, tmpTag);
    }
    XMLContents = string::replaceAll(XMLContents, targetTag, "wrongTag");
    if (!ignoreTag.empty()) {
      XMLContents = string::replaceAll(XMLContents, tmpTag, ignoreTag);
    }

    MAKE_RESULT_XML(MUT_DIR, XMLContents);
    Logger::setLevel(Logger::Level::DEBUG);
    testing::internal::CaptureStdout();
    auto mut = new Result(MUT_DIR);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(string::contains(out,
        "This file doesn't follow googletest result format:"));

    fs::remove_all(MUT_DIR);
    delete mut;
  }

  fs::path BASE;
  fs::path ORI_DIR;
  std::string TC1 =
      R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="1" failures="0" disabled="0" errors="0" time="*" timestamp="*" name="AllTests">
	<testsuite name="C1" tests="1" failures="0" skipped="0" disabled="0" errors="0" time="*" timestamp="*">
		<testcase name="TC1" status="run" result="completed" time="*" timestamp="*" classname="C1" />
	</testsuite>
</testsuites>
)";
  std::string TC2 =
      R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="1" failures="0" disabled="0" errors="0" time="*" timestamp="*" name="AllTests">
	<testsuite name="C2" tests="1" failures="0" skipped="0" disabled="0" errors="0" time="*" timestamp="*">
		<testcase name="TC2" status="run" result="completed" time="*" timestamp="*" classname="C2" />
	</testsuite>
</testsuites>
)";
  std::string TC2_FAIL =
      R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="1" failures="0" disabled="0" errors="0" time="*" timestamp="*" name="AllTests">
	<testsuite name="C2" tests="1" failures="0" skipped="0" disabled="0" errors="0" time="*" timestamp="*">
		<testcase name="TC2" status="run" result="completed" time="*" timestamp="*" classname="C2">
			<failure message="fail message" type="" />
		</testcase>
	</testsuite>
</testsuites>
)";
  std::string TC3 =
      R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="1" failures="0" disabled="0" errors="0" time="*" timestamp="*" name="AllTests">
	<testsuite name="C1" tests="1" failures="0" skipped="0" disabled="0" errors="0" time="*" timestamp="*">
		<testcase name="TC3" status="run" result="completed" time="*" timestamp="*" classname="C1" />
	</testsuite>
</testsuites>
)";
  std::string TC4_QT =
      R"asdf(<?xml version="1.0" encoding="UTF-8" ?>
<testsuite errors="0" failures="0" tests="4" name="qmake5-project.MinusTest">
  <properties>
    <property value="5.14.2" name="QTestVersion"/>
    <property value="5.14.2" name="QtVersion"/>
    <property value="Qt 5.14.2 (arm64&#x002D;little_endian&#x002D;lp64 shared (dynamic) release build; by GCC 9.3.0)" name="QtBuild"/>
  </properties>
  <testcase result="pass" name="initTestCase"/>
  <testcase result="pass" name="testShouldReturnExpectedValue"/>
  <testcase result="pass" name="testShouldAlsoFail"/>
  <testcase result="pass" name="cleanupTestCase"/>
  <system-err/>
</testsuite>
)asdf";
  std::string TC4_QT_FAIL =
      R"asdf(<?xml version="1.0" encoding="UTF-8" ?>
<testsuite errors="0" failures="1" tests="4" name="qmake5-project.MinusTest">
  <properties>
    <property value="5.14.2" name="QTestVersion"/>
    <property value="5.14.2" name="QtVersion"/>
    <property value="Qt 5.14.2 (arm64&#x002D;little_endian&#x002D;lp64 shared (dynamic) release build; by GCC 9.3.0)" name="QtBuild"/>
  </properties>
  <testcase result="pass" name="initTestCase"/>
  <testcase result="pass" name="testShouldReturnExpectedValue"/>
  <testcase result="fail" name="testShouldAlsoFail">
    <failure message="&apos;3 == arithmetic::minus(1, 2)&apos; returned FALSE. ()" result="fail"/>
  </testcase>
  <testcase result="pass" name="cleanupTestCase"/>
  <system-err/>
</testsuite>
)asdf";
};

TEST_F(ResultTest, testResultWithAliveMutation) {
  auto MUT_DIR = BASE / "mut_dir_alive_mutation";
  fs::create_directories(MUT_DIR);

  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "");
}

TEST_F(ResultTest, testResultWithKillMutation) {
  auto MUT_DIR = BASE / "mut_dir_kill_mutation";
  fs::create_directories(MUT_DIR);

  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithKillMutationUsingQtTestResult) {
  auto QT5_RESULT = BASE / "qt_ori_dir";
  fs::create_directories(QT5_RESULT);
  auto MUT_DIR = BASE / "mut_dir_kill_mutation_using_qt_test_result";
  fs::create_directories(MUT_DIR);
  MAKE_RESULT_XML(QT5_RESULT, TC4_QT);
  MAKE_RESULT_XML(MUT_DIR, TC4_QT_FAIL);

  Result ori(QT5_RESULT);
  Result mut(MUT_DIR);

  EXPECT_EQ(Result::kill(ori, mut),
      R"(qmake5-project.MinusTest.testShouldAlsoFail)");
}

TEST_F(ResultTest, testResultWithAliveMutationAddNewTC) {
  auto MUT_DIR = BASE / "mut_dir_alive_mutation_add_new_tc";
  fs::create_directories(MUT_DIR);
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2);
  MAKE_RESULT_XML(MUT_DIR, TC3);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "");
}

TEST_F(ResultTest, testResultWithKillMutationAddNewTC) {
  auto MUT_DIR = BASE / "mut_dir_kill_mutation_add_new_tc";
  fs::create_directories(MUT_DIR);
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);
  MAKE_RESULT_XML(MUT_DIR, TC3);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithKKillMutationErrorTC) {
  auto MUT_DIR = BASE / "mut_dir_kill_mutation_error_tc";
  fs::create_directories(MUT_DIR);
  MAKE_RESULT_XML(MUT_DIR, TC1);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithKKillMutationErrorTCAndAddNewTc) {
  auto MUT_DIR = BASE / "mut_dir_kill_mutation_error_tc_and_add_new_tc";
  fs::create_directories(MUT_DIR);
  MAKE_RESULT_XML(MUT_DIR, TC1);
  MAKE_RESULT_XML(MUT_DIR, TC3);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C2.TC2");
}

TEST_F(ResultTest, testResultWithEmptyMutationDir) {
  auto MUT_DIR = BASE / "mut_dir_kill_empty_mutation_dir";
  fs::create_directories(MUT_DIR);
  Result ori(ORI_DIR);
  Result mut(MUT_DIR);
  EXPECT_EQ(Result::kill(ori, mut), "C1.TC1, C2.TC2");
}

TEST_F(ResultTest, testResultWithWrongXMLFmt) {
  auto MUT_DIR = BASE / "mut_dir_kill_wrong_xml_fmt";
  fs::create_directories(MUT_DIR);
  MAKE_RESULT_XML(MUT_DIR, string::replaceAll(TC3, "</testsuites>", ""));

  Logger::setLevel(Logger::Level::DEBUG);
  testing::internal::CaptureStdout();
  auto mut = new Result(MUT_DIR);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out,
      "XML_ERROR_PARSING:"));
  delete mut;
}

TEST_F(ResultTest, testResultWithWrongResultFmt) {
  MAKE_AND_TEST_WRONG_RESULT_XML(TC3, "testsuites");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC3, "testsuite", "testsuites");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC3, "testcase");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC3, "status");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC3, "classname");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC3, "name", "classname");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC4_QT, "testsuite");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC4_QT, "testcase");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC4_QT, "result");
  MAKE_AND_TEST_WRONG_RESULT_XML(TC4_QT, "name");
}

}  // namespace sentinel
