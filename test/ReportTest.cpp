/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <fstream>
#include <sstream>
#include <string>
#include "helper/SentinelReportTestBase.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/Timestamper.hpp"
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
  EXPECT_TRUE(string::contains(out, "Mutation Score Report"));
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
  EXPECT_TRUE(string::contains(out2, "Mutation Score Report"));
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
  EXPECT_TRUE(string::contains(out, "Mutation Score Report"));
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
  EXPECT_TRUE(string::contains(out, "Mutation Score Report"));
  EXPECT_TRUE(string::contains(out, "33.3%"));
  EXPECT_TRUE(string::contains(out, "Skipped: 1 runtime error, 1 timeout"));
}

TEST_F(ReportTest, testMutationSummaryAggregatesTimeByState) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_TIME";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  MRs.back().setBuildSecs(1.5);
  MRs.back().setTestSecs(2.0);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "testB", "", MutationState::KILLED);
  MRs.back().setBuildSecs(0.5);
  MRs.back().setTestSecs(3.0);

  Mutant M3("AOR", REL_PATH3, "func", 4, 12, 4, 13, "*");
  MRs.emplace_back(M3, "", "", MutationState::SURVIVED);
  MRs.back().setBuildSecs(1.0);
  MRs.back().setTestSecs(4.0);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  // cppcheck-suppress unreadVariable
  MutationSummary summary(MRPath, SOURCE_DIR);

  // Verify per-state timing
  const auto& killed = summary.timeByState.at(MutationState::KILLED);
  EXPECT_DOUBLE_EQ(killed.buildSecs, 2.0);
  EXPECT_DOUBLE_EQ(killed.testSecs, 5.0);
  EXPECT_EQ(killed.count, 2u);
  EXPECT_EQ(killed.timedCount, 2u);

  const auto& survived = summary.timeByState.at(MutationState::SURVIVED);
  EXPECT_DOUBLE_EQ(survived.buildSecs, 1.0);
  EXPECT_DOUBLE_EQ(survived.testSecs, 4.0);
  EXPECT_EQ(survived.count, 1u);
  EXPECT_EQ(survived.timedCount, 1u);

  // Verify totals
  EXPECT_DOUBLE_EQ(summary.totalBuildSecs, 3.0);
  EXPECT_DOUBLE_EQ(summary.totalTestSecs, 9.0);
  EXPECT_EQ(summary.timedMutantCount, 3u);
}

TEST_F(ReportTest, testMutationSummaryPartialTiming) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_PARTIAL";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  MRs.back().setBuildSecs(1.5);
  MRs.back().setTestSecs(2.0);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);
  // No timing set — backward compat with old mt.done files

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  // cppcheck-suppress unreadVariable
  MutationSummary summary(MRPath, SOURCE_DIR);

  EXPECT_EQ(summary.timedMutantCount, 1u);
  EXPECT_EQ(summary.totNumberOfMutation, 2u);

  const auto& killed = summary.timeByState.at(MutationState::KILLED);
  EXPECT_EQ(killed.timedCount, 1u);
  EXPECT_EQ(killed.count, 1u);

  const auto& survived = summary.timeByState.at(MutationState::SURVIVED);
  EXPECT_EQ(survived.timedCount, 0u);
  EXPECT_EQ(survived.count, 1u);
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
  EXPECT_TRUE(string::contains(out, "Mutation Score Report"));
  EXPECT_TRUE(string::contains(out, "50.0%"));
  EXPECT_TRUE(string::contains(out, "Skipped: 1 build failure, 1 runtime error, 1 timeout"));
}

TEST_F(ReportTest, testPrintReportWithDurationSection) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_TIME_SECTION";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  MRs.back().setBuildSecs(10.0);
  MRs.back().setTestSecs(20.0);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);
  MRs.back().setBuildSecs(5.0);
  MRs.back().setTestSecs(15.0);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(string::contains(out, "Duration:"));
  EXPECT_TRUE(string::contains(out, "Killed"));
  EXPECT_TRUE(string::contains(out, "Survived"));
  EXPECT_FALSE(string::contains(out, "Skipped"));
  // Killed has 30s total (10+20), Survived has 20s (5+15)
  EXPECT_TRUE(string::contains(out, Timestamper::format(30.0)));
  EXPECT_TRUE(string::contains(out, Timestamper::format(20.0)));
}

TEST_F(ReportTest, testPrintReportWithoutTimingOmitsDurationSection) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_NO_TIME";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(string::contains(out, "Duration:"));
  EXPECT_FALSE(string::contains(out, "Duration ("));
}

TEST_F(ReportTest, testPrintReportWithPartialTimingShowsDurationCount) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_PARTIAL_TIME";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  MRs.back().setBuildSecs(1.5);
  MRs.back().setTestSecs(2.0);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);
  // No timing set

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(string::contains(out, "Duration (1/2 mutants):"));
}

TEST_F(ReportTest, testPrintReportWithLargeCountAlignsDurationAndStateRows) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_LARGE_COUNT";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::SURVIVED);
  MRs.back().setBuildSecs(1.5);
  MRs.back().setTestSecs(2.0);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);
  // No timing set — makes this a partial-timing scenario

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  MutationSummary summary(MRPath, SOURCE_DIR);

  // Inflate counts to simulate 5-digit mutant numbers (e.g., 3385/10376)
  summary.timedMutantCount = 3385;
  summary.totNumberOfMutation = 10376;

  ReportForTest report(summary);
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  // Find the Duration header line and the first state row beneath it.
  std::string durationLine;
  std::string stateLine;
  std::istringstream stream(out);
  std::string line;
  while (std::getline(stream, line)) {
    if (durationLine.empty()) {
      if (line.find("Duration (") != std::string::npos) {
        durationLine = line;
      }
    } else if (string::startsWith(line, "    ") &&
               line.find('%') != std::string::npos) {
      stateLine = line;
      break;
    }
  }

  ASSERT_FALSE(durationLine.empty()) << "Duration header line not found in output";
  ASSERT_FALSE(stateLine.empty()) << "Survived state row not found in output";

  // The '%' character must appear at the same column in both lines
  std::size_t durationPctPos = durationLine.find('%');
  std::size_t statePctPos = stateLine.find('%');
  ASSERT_NE(durationPctPos, std::string::npos) << "No '%' in duration line: " << durationLine;
  ASSERT_NE(statePctPos, std::string::npos) << "No '%' in state line: " << stateLine;
  EXPECT_EQ(durationPctPos, statePctPos)
      << "Duration header and state row '%' are misaligned\n"
      << "  Duration line: " << durationLine << "\n"
      << "  State line:    " << stateLine;
}

TEST_F(ReportTest, testPrintReportWithDurationAndSkipped) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_TIME_SKIP";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;

  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  MRs.back().setBuildSecs(10.0);
  MRs.back().setTestSecs(20.0);

  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::BUILD_FAILURE);
  MRs.back().setBuildSecs(8.0);
  MRs.back().setTestSecs(0.0);

  Mutant M3("AOR", REL_PATH3, "func", 4, 12, 4, 13, "*");
  MRs.emplace_back(M3, "", "", MutationState::TIMEOUT);
  MRs.back().setBuildSecs(3.0);
  MRs.back().setTestSecs(90.0);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(string::contains(out, "Duration:"));
  EXPECT_TRUE(string::contains(out, "Killed"));
  EXPECT_TRUE(string::contains(out, "Timeout"));
  EXPECT_TRUE(string::contains(out, "Build Failure"));
  EXPECT_TRUE(string::contains(out, "Skipped:"));
  // Survived and Runtime Error have 0 timed mutants, should be omitted
  // Need to check that "Survived" and "Runtime Error" do NOT appear in the Duration section
  // But "Survived" appears in the header row, so check it doesn't appear after "Duration:"
  auto durationPos = out.find("Duration:");
  ASSERT_NE(durationPos, std::string::npos);
  std::string durationSection = out.substr(durationPos);
  EXPECT_FALSE(string::contains(durationSection, "Survived"));
  EXPECT_FALSE(string::contains(durationSection, "Runtime Error"));
}

TEST_F(ReportTest, testPrintReportOnlyBuildFailures) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_ONLY_BUILD_FAIL";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;
  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);
  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::BUILD_FAILURE);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(string::contains(out, "-%"));
  EXPECT_TRUE(string::contains(out, "Skipped: 2 build failures"));
}

TEST_F(ReportTest, testPrintReportOnlyRuntimeErrors) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_ONLY_RT_ERR";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;
  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::RUNTIME_ERROR);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(string::contains(out, "Skipped: 1 runtime error"));
  EXPECT_FALSE(string::contains(out, "build failure"));
  EXPECT_FALSE(string::contains(out, "timeout"));
}

TEST_F(ReportTest, testPrintReportOnlyTimeouts) {
  auto MUT_RESULT_DIR = BASE / "MUT_RESULT_DIR_ONLY_TIMEOUT";
  fs::create_directories(MUT_RESULT_DIR);

  MutationResults MRs;
  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::TIMEOUT);
  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::TIMEOUT);
  Mutant M3("AOR", REL_PATH3, "func", 4, 12, 4, 13, "*");
  MRs.emplace_back(M3, "", "", MutationState::TIMEOUT);

  auto MRPath = MUT_RESULT_DIR / "MutationResult";
  MRs.save(MRPath);

  ReportForTest report(MutationSummary(MRPath, SOURCE_DIR));
  testing::internal::CaptureStdout();
  report.printSummary();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(string::contains(out, "Skipped: 3 timeouts"));
  EXPECT_FALSE(string::contains(out, "build failure"));
  EXPECT_FALSE(string::contains(out, "runtime error"));
}

TEST_F(ReportTest, testMutationSummaryCopyConstructorEmptyResults) {
  MutationResults MRs;
  MutationSummary original(MRs, SOURCE_DIR);
  MutationSummary copy(original);

  EXPECT_EQ(0u, copy.totNumberOfMutation);
  EXPECT_EQ(0u, copy.totNumberOfDetectedMutation);
  EXPECT_TRUE(copy.groupByPath.empty());
  EXPECT_TRUE(copy.groupByDirPath.empty());
}

TEST_F(ReportTest, testMutationSummaryCopyConstructorWithResults) {
  MutationResults MRs;
  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);

  MutationSummary original(MRs, SOURCE_DIR);
  MutationSummary copy(original);

  EXPECT_EQ(original.totNumberOfMutation, copy.totNumberOfMutation);
  EXPECT_EQ(original.totNumberOfDetectedMutation, copy.totNumberOfDetectedMutation);
  EXPECT_EQ(original.groupByPath.size(), copy.groupByPath.size());
  EXPECT_EQ(original.groupByDirPath.size(), copy.groupByDirPath.size());
}

TEST_F(ReportTest, testMutationSummaryAssignmentOperator) {
  MutationResults MRs;
  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);

  MutationSummary original(MRs, SOURCE_DIR);
  MutationSummary assigned(MutationResults(), SOURCE_DIR);
  assigned = original;

  EXPECT_EQ(original.totNumberOfMutation, assigned.totNumberOfMutation);
  EXPECT_EQ(original.totNumberOfDetectedMutation, assigned.totNumberOfDetectedMutation);
}

TEST_F(ReportTest, testMutationSummaryAggregatesSkippedStates) {
  MutationResults MRs;
  Mutant M1("AOR", REL_PATH1, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "", "", MutationState::BUILD_FAILURE);
  MRs.back().setBuildSecs(1.0);
  Mutant M2("AOR", REL_PATH2, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::RUNTIME_ERROR);
  Mutant M3("AOR", REL_PATH3, "func", 4, 12, 4, 13, "*");
  MRs.emplace_back(M3, "", "", MutationState::TIMEOUT);

  MutationSummary summary(MRs, SOURCE_DIR);
  EXPECT_EQ(0u, summary.totNumberOfMutation);
  EXPECT_EQ(0u, summary.totNumberOfDetectedMutation);
  EXPECT_EQ(1u, summary.totNumberOfBuildFailure);
  EXPECT_EQ(1u, summary.totNumberOfRuntimeError);
  EXPECT_EQ(1u, summary.totNumberOfTimeout);
  EXPECT_TRUE(summary.groupByPath.empty());
  EXPECT_TRUE(summary.groupByDirPath.empty());
}

TEST_F(ReportTest, testMutationSummaryConstructorFromFileThrowsOnDirectory) {
  auto resultDir = BASE / "result_dir_not_file";
  fs::create_directories(resultDir);
  EXPECT_THROW(MutationSummary(resultDir, SOURCE_DIR), InvalidArgumentException);
}

TEST_F(ReportTest, testMutationSummaryDirGroupFileCount) {
  MutationResults MRs;
  Mutant M1("AOR", REL_PATH2, "func", 2, 12, 2, 13, "+");
  MRs.emplace_back(M1, "testA", "", MutationState::KILLED);
  Mutant M2("AOR", REL_PATH3, "func", 3, 12, 3, 13, "-");
  MRs.emplace_back(M2, "", "", MutationState::SURVIVED);

  MutationSummary summary(MRs, SOURCE_DIR);
  auto it = summary.groupByDirPath.find(REL_PATH2.parent_path());
  ASSERT_NE(summary.groupByDirPath.end(), it);
  EXPECT_EQ(2u, it->second.fileCount);
}

}  // namespace sentinel
