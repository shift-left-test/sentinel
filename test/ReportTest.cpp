/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include "helper/SentinelReportTestBase.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class ReportTest : public SentinelReportTestBase {
 protected:
  fs::path REL_PATH1 = fs::path("NESTED_DIR1/NESTED_DIR") / "target1_veryVeryVeryVeryVerylongFilePath.cpp";
  fs::path REL_PATH2 = fs::path("NESTED_DIR2") / "target2.cpp";
  fs::path REL_PATH3 = fs::path("NESTED_DIR2") / "target3.cpp";
  fs::path REL_PATH4 = fs::path("target4.cpp");

  void SetUp() override {
    setUpDirectories("SENTINEL_REPORTTEST_TMP_DIR");
    makeFile(TARGET_FULL_PATH);
    makeFile(TARGET_FULL_PATH2);
    makeFile(TARGET_FULL_PATH3);
    makeFile(TARGET_FULL_PATH4);
  }

  void TearDown() override {
    tearDownBase();
  }

  void makeFile(const std::string& path) {
    std::ofstream(path).close();
  }
};

class ReportForTest : public Report {
 public:
  explicit ReportForTest(const MutationSummary& summary) : Report(summary) {}
  void save(const std::filesystem::path& dirPath) override {
    if (fs::exists(dirPath)) {
      return;
    }
  }
};

TEST_F(ReportTest, testPrintEmptyReport) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESLUT_DIR_0";
  fs::create_directories(MUT_RESULT_DIR);
  auto MRPath = MUT_RESULT_DIR / "MutationResult";

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(!string::contains(out, "Ignored Mutation"));
  EXPECT_TRUE(string::contains(out, "Mutation Coverage Report"));
  EXPECT_TRUE(string::contains(out, "Killed"));
  EXPECT_TRUE(string::contains(out, "Survived"));
  EXPECT_TRUE(string::contains(out, "TOTAL"));
  EXPECT_TRUE(string::contains(out, "-%"));
  EXPECT_FALSE(string::contains(out, "Skipped"));

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);

  Mutant M2("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M2, "", "", MutationState::RUNTIME_ERROR);

  Mutant M3("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M3, "", "", MutationState::TIMEOUT);

  MRs.save(MRPath);

  ReportForTest report2(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report2.printSummary();
  std::string out2 = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out2, "Mutation Coverage Report"));
  EXPECT_TRUE(string::contains(out2, "TOTAL"));
  EXPECT_TRUE(string::contains(out2, "-%"));
  EXPECT_TRUE(string::contains(out2, "Skipped: 1 build failure, 1 runtime error, 1 timeout"));
}

TEST_F(ReportTest, testPrintReportWithNoRuntimeerrorAndNoBuildFailure) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_1";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", REL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", REL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::KILLED);

  Mutant M4("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M4, "", "", MutationState::SURVIVED);

  Mutant M5("AOR", REL_PATH4, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));

  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(!string::contains(out, "Ignored Mutation"));
  EXPECT_TRUE(string::contains(out, "Mutation Coverage Report"));
  EXPECT_TRUE(string::contains(out, "target2.cpp"));
  EXPECT_TRUE(string::contains(out, "target3.cpp"));
  EXPECT_TRUE(string::contains(out, "target4.cpp"));
  EXPECT_TRUE(string::contains(out, "40.0%"));
  EXPECT_FALSE(string::contains(out, "Skipped"));
}

TEST_F(ReportTest, testPrintReportWithRuntimeerrorAndTimeout) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_3";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::SURVIVED);

  Mutant M2("BOR", REL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", REL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::RUNTIME_ERROR);

  Mutant M5("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  Mutant M6("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M6, "", "", MutationState::TIMEOUT);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));

  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out, "Mutation Coverage Report"));
  EXPECT_TRUE(string::contains(out, "33.3%"));
  EXPECT_TRUE(string::contains(out, "Skipped: 1 runtime error, 1 timeout"));
}

TEST_F(ReportTest, testPrintReportWithRuntimeerrorAndBuildFailureAndTimeout) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_4";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);

  Mutant M2("BOR", REL_PATH2, "sumOfEvenPositiveNumber", 2, 12, 2, 13, "|");
  MRs.emplace_back(M2, "testBitwiseOR", "", MutationState::KILLED);

  Mutant M3("BOR", REL_PATH3, "sumOfEvenPositiveNumber", 3, 12, 3, 13, "&");
  MRs.emplace_back(M3, "testBitwiseAND, testBitwiseOP", "", MutationState::RUNTIME_ERROR);

  Mutant M5("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M5, "", "", MutationState::SURVIVED);

  Mutant M6("AOR", REL_PATH3, "sumOfEvenPositiveNumber", 8, 12, 8, 13, "-");
  MRs.emplace_back(M6, "", "", MutationState::TIMEOUT);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));

  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(string::contains(out, "Mutation Coverage Report"));
  EXPECT_TRUE(string::contains(out, "50.0%"));
  EXPECT_TRUE(string::contains(out, "Skipped: 1 build failure, 1 runtime error, 1 timeout"));
}

}  // namespace sentinel
