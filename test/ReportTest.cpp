/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <string>
#include <sstream>
#include "helper/CaptureHelper.hpp"
#include "helper/SentinelReportTestBase.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

class ReportTest : public SentinelReportTestBase {
 protected:
  void SetUp() override {
    setUpDirectories("SENTINEL_REPORTTEST_TMP_DIR");
    makeFile(TARGET_FULL_PATH);
    makeFile(TARGET_FULL_PATH2);
    makeFile(TARGET_FULL_PATH3);
    makeFile(TARGET_FULL_PATH4);
    mStdoutCapture = CaptureHelper::getStdoutCapture();
  }

  void TearDown() override {
    tearDownBase();
  }

  void makeFile(const std::string& path) {
    std::ofstream(path).close();
  }

  void captureStdout() {
    mStdoutCapture->capture();
  }

  std::string capturedStdout() {
    return mStdoutCapture->release();
  }

 private:
  std::shared_ptr<CaptureHelper> mStdoutCapture;
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
  captureStdout();
  report.printSummary();
  std::string out = capturedStdout();
  EXPECT_TRUE(!string::contains(out, "Ignored Mutation"));
  EXPECT_EQ(
      R"a1b2z(==================================================================================
                             Mutation Coverage Report
==================================================================================
File                                                  Killed     Total     Score
----------------------------------------------------------------------------------
----------------------------------------------------------------------------------
TOTAL                                                      0         0        -%
==================================================================================
)a1b2z",
      out);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);

  Mutant M2("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M2, "", "", MutationState::RUNTIME_ERROR);

  Mutant M3("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M3, "", "", MutationState::TIMEOUT);

  MRs.save(MRPath);

  ReportForTest report2(MRPath, SOURCE_DIR);
  captureStdout();
  report2.printSummary();
  std::string out2 = capturedStdout();
  EXPECT_EQ(
      R"a1b2z(==================================================================================
                             Mutation Coverage Report
==================================================================================
File                                                  Killed     Total     Score
----------------------------------------------------------------------------------
----------------------------------------------------------------------------------
TOTAL                                                      0         0        -%
==================================================================================
)a1b2z"
      "Skipped: 1 build failure  \xc2\xb7  1 runtime error  \xc2\xb7  1 timeout\n"
      "==================================================================================\n",
      out2);
}

TEST_F(ReportTest, testPrintReportWithNoRuntimeerrorAndNoBuildFailure) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_1";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::KILLED);

  Mutant M4("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M4, "", "", MutationState::SURVIVED);

  Mutant M5("AOR", TARGET_FULL_PATH4, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MRPath, SOURCE_DIR);

  captureStdout();
  report.printSummary();
  std::string out = capturedStdout();
  EXPECT_TRUE(!string::contains(out, "Ignored Mutation"));
  EXPECT_EQ(
      R"a1b2z(==================================================================================
                             Mutation Coverage Report
==================================================================================
File                                                  Killed     Total     Score
----------------------------------------------------------------------------------
... R/target1_veryVeryVeryVeryVerylongFilePath.cpp         0         1      0.0%
NESTED_DIR2/target2.cpp                                    1         1    100.0%
NESTED_DIR2/target3.cpp                                    1         2     50.0%
target4.cpp                                                0         1      0.0%
----------------------------------------------------------------------------------
TOTAL                                                      2         5     40.0%
==================================================================================
)a1b2z",
      out);
}

TEST_F(ReportTest, testPrintReportWithRuntimeerrorAndTimeout) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_3";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::RUNTIME_ERROR);

  Mutant M5("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  Mutant M6("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M6, "", "", MutationState::TIMEOUT);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MRPath, SOURCE_DIR);

  captureStdout();
  report.printSummary();
  std::string out = capturedStdout();
  EXPECT_EQ(
      R"a1b2z(==================================================================================
                             Mutation Coverage Report
==================================================================================
File                                                  Killed     Total     Score
----------------------------------------------------------------------------------
... R/target1_veryVeryVeryVeryVerylongFilePath.cpp         0         1      0.0%
NESTED_DIR2/target2.cpp                                    1         1    100.0%
NESTED_DIR2/target3.cpp                                    0         1      0.0%
----------------------------------------------------------------------------------
TOTAL                                                      1         3     33.3%
==================================================================================
)a1b2z"
      "Skipped: 1 runtime error  \xc2\xb7  1 timeout\n"
      "==================================================================================\n",
      out);
}

TEST_F(ReportTest, testPrintReportWithRuntimeerrorAndBuildFailureAndTimeout) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_4";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", TARGET_FULL_PATH, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);

  Mutant M2("BOR", TARGET_FULL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::RUNTIME_ERROR);

  Mutant M5("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  Mutant M6("AOR", TARGET_FULL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M6, "", "", MutationState::TIMEOUT);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MRPath, SOURCE_DIR);

  captureStdout();
  report.printSummary();
  std::string out = capturedStdout();
  EXPECT_EQ(
      R"a1b2z(==================================================================================
                             Mutation Coverage Report
==================================================================================
File                                                  Killed     Total     Score
----------------------------------------------------------------------------------
NESTED_DIR2/target2.cpp                                    1         1    100.0%
NESTED_DIR2/target3.cpp                                    0         1      0.0%
----------------------------------------------------------------------------------
TOTAL                                                      1         2     50.0%
==================================================================================
)a1b2z"
      "Skipped: 1 build failure  \xc2\xb7  1 runtime error  \xc2\xb7  1 timeout\n"
      "==================================================================================\n",
      out);
}

}  // namespace sentinel
