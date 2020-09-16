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
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/util/filesystem.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

class EvaluatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = util::filesystem::tempDirectory("fixture");
    OUT_DIR = util::filesystem::tempDirectory(util::filesystem::join(BASE,
        "OUT_DIR"));

    ORI_DIR = util::filesystem::tempDirectory(util::filesystem::join(BASE,
        "ORI_DIR"));
    MAKE_RESULT_XML(ORI_DIR, TC1);
    MAKE_RESULT_XML(ORI_DIR, TC2);

    MUT_DIR = util::filesystem::tempDirectory(util::filesystem::join(BASE,
        "MUT_DIR"));
    MAKE_RESULT_XML(MUT_DIR, TC1);
    MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);

    MUT_DIR_ALIVE = util::filesystem::tempDirectory(util::filesystem::join(BASE,
        "MUT_DIR_ALIVE"));
    MAKE_RESULT_XML(MUT_DIR_ALIVE, TC1);
    MAKE_RESULT_XML(MUT_DIR_ALIVE, TC2);

    MUTABLEDB = util::filesystem::join(BASE, "mutables.db");

    m = new Mutables(MUTABLEDB);
    Mutable mutable2("AOR", "input/sample1/sample1.cpp",
                     "sumOfEvenPositiveNumber", 0, 0, 0, 0, "+");
    Mutable mutable1("BOR", "input/sample1/sample1.cpp",
                     "sumOfEvenPositiveNumber", 1, 1, 1, 1, "|");
    m->add(mutable1);
    m->add(mutable2);
    m->save();
  }

  void TearDown() override {
    util::filesystem::removeDirectories(BASE);
    delete m;
  }

  void MAKE_RESULT_XML(const std::string& dirPath,
      const std::string& fileContent) {
    std::string tmp = util::filesystem::tempFilenameWithSuffix(
        util::filesystem::join(dirPath, "pre"), ".xml");
    std::ofstream tmpfile;
    tmpfile.open(tmp.c_str());
    tmpfile << fileContent.c_str();
    tmpfile.close();
  }

  Mutables* m = nullptr;
  std::string BASE;
  std::string ORI_DIR;
  std::string OUT_DIR;
  std::string MUT_DIR;
  std::string MUT_DIR_ALIVE;
  std::string MUTABLEDB;
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
};

TEST_F(EvaluatorTest, testEvaluatorWithKilledMutation) {
  auto mLogger = Logger::getLogger("EvaluatorTest");
  mLogger->setLevel(Logger::Level::DEBUG);
  testing::internal::CaptureStdout();
  Evaluator mEvaluator(MUTABLEDB, ORI_DIR, OUT_DIR, mLogger);
  std::string out1 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(util::string::contains(out1, "Load mutable DB: "));
  EXPECT_TRUE(util::string::contains(out1, "Load Expected Result: "));

  testing::internal::CaptureStdout();
  auto result = mEvaluator.compareAndSaveMutationResult(MUT_DIR, 1);
  std::string out2 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(util::string::contains(out2, "Load mutable idx: 1"));
  EXPECT_TRUE(util::string::contains(out2, "Load Actual Result: "));
  EXPECT_TRUE(util::string::contains(out2, "killing TC: C2.TC2"));
  EXPECT_TRUE(util::string::contains(out2, "Save MutationResult:"));
  EXPECT_TRUE(result.getDetected());

  MutationResult mr(util::filesystem::join(OUT_DIR, "1.MutationResult"));
  EXPECT_TRUE(mr.compare(result));
}

TEST_F(EvaluatorTest, testEvaluatorWithAlivededMutation) {
  auto mLogger = Logger::getLogger("EvaluatorTest");
  mLogger->setLevel(Logger::Level::DEBUG);
  testing::internal::CaptureStdout();
  Evaluator mEvaluator(MUTABLEDB, ORI_DIR, OUT_DIR, mLogger);
  std::string out1 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(util::string::contains(out1, "Load mutable DB: "));
  EXPECT_TRUE(util::string::contains(out1, "Load Expected Result: "));

  testing::internal::CaptureStdout();
  auto result = mEvaluator.compareAndSaveMutationResult(MUT_DIR_ALIVE, 0);
  std::string out2 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(util::string::contains(out2, "Load mutable idx: 0"));
  EXPECT_TRUE(util::string::contains(out2, "Load Actual Result: "));
  EXPECT_TRUE(util::string::contains(out2, "Save MutationResult:"));
  EXPECT_FALSE(result.getDetected());

  MutationResult mr(util::filesystem::join(OUT_DIR, "0.MutationResult"));
  EXPECT_TRUE(mr.compare(result));
}

}  // namespace sentinel
