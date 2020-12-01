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
#include "sentinel/Report.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/string.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class ReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = fs::temp_directory_path() / "SENTINEL_REPORTTEST_TMP_DIR";
    fs::remove_all(BASE);

    SOURCE_DIR = BASE / "SOURCE_DIR";
    fs::create_directories(SOURCE_DIR);
    NESTED_SOURCE_DIR = SOURCE_DIR / "NESTED_DIR1/NESTED_DIR";
    fs::create_directories(NESTED_SOURCE_DIR);
    NESTED_SOURCE_DIR2 = SOURCE_DIR / "NESTED_DIR2";
    fs::create_directories(NESTED_SOURCE_DIR2);


    TARGET_FULL_PATH =
        NESTED_SOURCE_DIR / "target1_veryVeryVeryVeryVerylongFilePath.cpp";
    makeFile(TARGET_FULL_PATH);
    TARGET_FULL_PATH2 = NESTED_SOURCE_DIR2 / "target2.cpp";
    makeFile(TARGET_FULL_PATH2);
    TARGET_FULL_PATH3 = NESTED_SOURCE_DIR2 / "target3.cpp";
    makeFile(TARGET_FULL_PATH3);
    TARGET_FULL_PATH4 = SOURCE_DIR / "target4.cpp";
    makeFile(TARGET_FULL_PATH4);
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  void makeFile(const std::string& path) {
    std::ofstream(path).close();
  }

  fs::path BASE;
  fs::path SOURCE_DIR;
  fs::path NESTED_SOURCE_DIR;
  fs::path NESTED_SOURCE_DIR2;
  fs::path TARGET_FULL_PATH;
  fs::path TARGET_FULL_PATH2;
  fs::path TARGET_FULL_PATH3;
  fs::path TARGET_FULL_PATH4;
};

class ReportForTest : public Report {
 public:
  using Report::Report;
  void save(const std::experimental::filesystem::path& dirPath) override {
    if (fs::exists(dirPath)) {
      return;
    }
  }
};

TEST_F(ReportTest, testPrintEmptyReport) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESLUT_DIR_0";
  fs::create_directories(MUT_RESULT_DIR);
  auto MRPath = MUT_RESULT_DIR / "MutationResult";

  ReportForTest report(MRPath, SOURCE_DIR);
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(!string::contains(out, "Ignored Mutation"));
  EXPECT_EQ(
      R"a1b2z(----------------------------------------------------------------------------------
                             Mutation Coverage Report                             
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
----------------------------------------------------------------------------------
TOTAL                                                      0         0        -%
----------------------------------------------------------------------------------
)a1b2z", out);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);

  Mutant M2("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MRs.emplace_back(M2, "", "", MutationState::RUNTIME_ERROR);

  MRs.save(MRPath);

  ReportForTest report2(MRPath, SOURCE_DIR);
  testing::internal::CaptureStdout();
  report2.printSummary();
  std::string out2 = testing::internal::GetCapturedStdout();
  EXPECT_EQ(
      R"a1b2z(----------------------------------------------------------------------------------
                             Mutation Coverage Report                             
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
----------------------------------------------------------------------------------
TOTAL                                                      0         0        -%
----------------------------------------------------------------------------------
Ignored Mutation
Build Failure                                                        1          
Runtime Error                                                        1          
----------------------------------------------------------------------------------
)a1b2z", out2);
}

TEST_F(ReportTest, testPrintReportWithNoRuntimeerrorAndNoBuildFailure) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_1";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "",
                     MutationState::KILLED);

  Mutant M4("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M4, "", "", MutationState::SURVIVED);

  Mutant M5("AOR", TARGET_FULL_PATH4, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MRPath, SOURCE_DIR);

  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(!string::contains(out, "Ignored Mutation"));
  EXPECT_EQ(
      R"a1b2z(----------------------------------------------------------------------------------
                             Mutation Coverage Report                             
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
... R/target1_veryVeryVeryVeryVerylongFilePath.cpp         0         1        0%
NESTED_DIR2/target2.cpp                                    1         1      100%
NESTED_DIR2/target3.cpp                                    1         2       50%
target4.cpp                                                0         1        0%
----------------------------------------------------------------------------------
TOTAL                                                      2         5       40%
----------------------------------------------------------------------------------
)a1b2z", out);
}

TEST_F(ReportTest, testPrintReportWithRuntimeerrorAndNoBuildFailure) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_3";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "",
                     MutationState::RUNTIME_ERROR);

  Mutant M5("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MRPath, SOURCE_DIR);

  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(
      R"a1b2z(----------------------------------------------------------------------------------
                             Mutation Coverage Report                             
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
... R/target1_veryVeryVeryVeryVerylongFilePath.cpp         0         1        0%
NESTED_DIR2/target2.cpp                                    1         1      100%
NESTED_DIR2/target3.cpp                                    0         1        0%
----------------------------------------------------------------------------------
TOTAL                                                      1         3       33%
----------------------------------------------------------------------------------
Ignored Mutation
Build Failure                                                        0          
Runtime Error                                                        1          
----------------------------------------------------------------------------------
)a1b2z", out);
}

TEST_F(ReportTest, testPrintReportWithRuntimeerrorAndBuildFailure) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_4";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber",
             2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "",
                     MutationState::RUNTIME_ERROR);

  Mutant M5("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber",
             8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MRPath, SOURCE_DIR);

  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(
      R"a1b2z(----------------------------------------------------------------------------------
                             Mutation Coverage Report                             
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
NESTED_DIR2/target2.cpp                                    1         1      100%
NESTED_DIR2/target3.cpp                                    0         1        0%
----------------------------------------------------------------------------------
TOTAL                                                      1         2       50%
----------------------------------------------------------------------------------
Ignored Mutation
Build Failure                                                        1          
Runtime Error                                                        1          
----------------------------------------------------------------------------------
)a1b2z", out);
}

}  // namespace sentinel
