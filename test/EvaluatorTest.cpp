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
#include "sentinel/MutationResults.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

class EvaluatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = os::tempDirectory("fixture");
    OUT_DIR = os::tempDirectory(os::path::join(BASE,
        "OUT_DIR"));

    ORI_DIR = os::tempDirectory(os::path::join(BASE,
        "ORI_DIR"));
    MAKE_RESULT_XML(ORI_DIR, TC1);
    MAKE_RESULT_XML(ORI_DIR, TC2);

    MUT_DIR = os::tempDirectory(os::path::join(BASE,
        "MUT_DIR"));
    MAKE_RESULT_XML(MUT_DIR, TC1);
    MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);

    MUT_DIR_ALIVE = os::tempDirectory(os::path::join(BASE,
        "MUT_DIR_ALIVE"));
    MAKE_RESULT_XML(MUT_DIR_ALIVE, TC1);
    MAKE_RESULT_XML(MUT_DIR_ALIVE, TC2);

    mutable1 = new Mutable("AOR", "input/sample1/sample1.cpp",
                     "sumOfEvenPositiveNumber", 0, 0, 0, 0, "+");
    mutable2 = new Mutable("BOR", "input/sample1/sample1.cpp",
                     "sumOfEvenPositiveNumber", 1, 1, 1, 1, "|");
  }

  void TearDown() override {
    os::removeDirectories(BASE);
    delete mutable1;
    delete mutable2;
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

  Mutable* mutable1 = nullptr;
  Mutable* mutable2 = nullptr;
  std::string BASE;
  std::string ORI_DIR;
  std::string OUT_DIR;
  std::string MUT_DIR;
  std::string MUT_DIR_ALIVE;
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
  Evaluator mEvaluator(*mutable1, ORI_DIR, OUT_DIR);

  testing::internal::CaptureStdout();
  auto result = mEvaluator.compareAndSaveMutationResult(MUT_DIR);
  std::string out2 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out2, "AOR("));
  EXPECT_TRUE(string::contains(out2, "sample1.cpp, 0:0-0:0) Killed by C2.TC2"));
  EXPECT_TRUE(result.getDetected());

  auto mrPath = os::path::join(OUT_DIR, "MutationResult");
  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  os::removeFile(mrPath);
}

TEST_F(EvaluatorTest, testEvaluatorWithAlivededMutation) {
  Evaluator mEvaluator(*mutable2, ORI_DIR, OUT_DIR);

  testing::internal::CaptureStdout();
  auto result = mEvaluator.compareAndSaveMutationResult(MUT_DIR_ALIVE);
  std::string out2 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out2, "BOR("));
  EXPECT_TRUE(string::contains(out2, "sample1.cpp, 1:1-1:1) Survived"));
  EXPECT_FALSE(result.getDetected());

  auto mrPath = os::path::join(OUT_DIR, "MutationResult");
  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  os::removeFile(mrPath);
}

}  // namespace sentinel
