/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include "helper/CaptureHelper.hpp"
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

class EvaluatorTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    BASE = fs::temp_directory_path() / "SENTINEL_EVALUATORTEST_TMP_DIR";
    fs::remove_all(BASE);

    OUT_DIR = BASE / "OUT_DIR";
    fs::create_directories(OUT_DIR);
    ORI_DIR = BASE / "ORI_DIR";
    fs::create_directories(ORI_DIR);

    MAKE_RESULT_XML(ORI_DIR, TC1);
    MAKE_RESULT_XML(ORI_DIR, TC2);

    ORI_DIR_FAIL = BASE / "ORI_DIR_FAIL";
    fs::create_directories(ORI_DIR_FAIL);
    MAKE_RESULT_XML(ORI_DIR_FAIL, TC2_FAIL);

    MUT_DIR = BASE / "MUT_DIR";
    fs::create_directories(MUT_DIR);
    MAKE_RESULT_XML(MUT_DIR, TC1);
    MAKE_RESULT_XML(MUT_DIR, TC2_FAIL);

    MUT_DIR_SURVIVED = BASE / "MUT_DIR_SURVIVED";
    fs::create_directories(MUT_DIR_SURVIVED);
    MAKE_RESULT_XML(MUT_DIR_SURVIVED, TC1);
    MAKE_RESULT_XML(MUT_DIR_SURVIVED, TC2);

    mutable1 = new Mutant("AOR", SAMPLE1_PATH, "sumOfEvenPositiveNumber", 0, 0, 0, 0, "+");
    auto SAMPLE1_CLONE_PATH = SAMPLE1_DIR / "veryVeryVeryVeryVeryVeryVeryVeryVeryLongSampleFile.cpp";
    fs::copy(SAMPLE1_PATH, SAMPLE1_CLONE_PATH);
    mutable2 = new Mutant("BOR", SAMPLE1_CLONE_PATH, "sumOfEvenPositiveNumber", 1, 1, 1, 1, "|");

    mStdoutCapture = CaptureHelper::getStdoutCapture();
  }

  void TearDown() override {
    fs::remove_all(BASE);
    delete mutable1;
    delete mutable2;
    SampleFileGeneratorForTest::TearDown();
  }

  void MAKE_RESULT_XML(const fs::path& dirPath, const std::string& fileContent) {
    auto tmp = dirPath / "pre.xml";
    std::ofstream tmpfile;
    tmpfile.open(tmp.c_str());
    tmpfile << fileContent.c_str();
    tmpfile.close();
  }

  void captureStdout() {
    mStdoutCapture->capture();
  }

  std::string capturedStdout() {
    return mStdoutCapture->release();
  }

  Mutant* mutable1 = nullptr;
  Mutant* mutable2 = nullptr;
  Mutant* mutable3 = nullptr;
  Mutant* mutable4 = nullptr;
  fs::path BASE;
  std::string ORI_DIR;
  std::string ORI_DIR_FAIL;
  fs::path OUT_DIR;
  std::string MUT_DIR;
  std::string MUT_DIR_SURVIVED;
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

 private:
  std::shared_ptr<CaptureHelper> mStdoutCapture;
};

TEST_F(EvaluatorTest, testConstructorFailWhenInvalidOutDirGiven) {
  auto mrPath = OUT_DIR / "MutationResult";
  EXPECT_NO_THROW(Evaluator(ORI_DIR, SAMPLE_BASE)
                  .compareAndSaveMutationResult(*mutable1, MUT_DIR, mrPath, "success"));

  auto mrPathForException = SAMPLE1_PATH / "MutationResult";
  EXPECT_THROW(Evaluator(ORI_DIR, SAMPLE_BASE)
               .compareAndSaveMutationResult(*mutable1, MUT_DIR, mrPathForException, "success"),
               InvalidArgumentException);
}

TEST_F(EvaluatorTest, testConstructorFailWhenNoPassedTCInGivenResult) {
  auto emptyPath = BASE / "EMPTY_DIR";
  fs::create_directories(emptyPath);
  EXPECT_THROW(Evaluator(emptyPath, SAMPLE_BASE), InvalidArgumentException);

  EXPECT_THROW(Evaluator(ORI_DIR_FAIL, SAMPLE_BASE), InvalidArgumentException);
}

TEST_F(EvaluatorTest, testEvaluatorWithKilledMutation) {
  Evaluator mEvaluator(ORI_DIR, SAMPLE_BASE);

  captureStdout();
  auto mrPath = OUT_DIR / "MutationResult";
  auto result = mEvaluator.compareAndSaveMutationResult(*mutable1, MUT_DIR, mrPath, "success");
  std::string out2 = capturedStdout();
  EXPECT_TRUE(string::contains(out2, "AOR : "));
  EXPECT_TRUE(string::contains(out2, ".cpp (0:0-0:0 -> +)"));
  EXPECT_TRUE(string::contains(out2, "KILLED"));
  EXPECT_TRUE(result.getDetected());

  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  fs::remove(mrPath);
}

TEST_F(EvaluatorTest, testEvaluatorWithSurvivedMutation) {
  Evaluator mEvaluator(ORI_DIR, SAMPLE_BASE);

  captureStdout();
  auto mrPath = OUT_DIR / "newDir" / "MutationResult";
  auto result = mEvaluator.compareAndSaveMutationResult(*mutable2, MUT_DIR_SURVIVED, mrPath, "success");
  std::string out2 = capturedStdout();
  EXPECT_TRUE(string::contains(out2, "BOR : "));
  EXPECT_TRUE(string::contains(out2, ".cpp (1:1-1:1 -> |)"));
  EXPECT_TRUE(string::contains(out2, "SURVIVED"));
  EXPECT_FALSE(result.getDetected());

  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  fs::remove(mrPath);
}

TEST_F(EvaluatorTest, testEvaluatorWithUncoveredMutation) {
  Evaluator mEvaluator(ORI_DIR, SAMPLE_BASE);

  captureStdout();
  auto mrPath = OUT_DIR / "newDir" / "MutationResult";
  auto result = mEvaluator.compareAndSaveMutationResult(*mutable2, MUT_DIR_SURVIVED, mrPath, "uncovered");
  std::string out2 = capturedStdout();
  EXPECT_TRUE(string::contains(out2, "BOR : "));
  EXPECT_TRUE(string::contains(out2, ".cpp (1:1-1:1 -> |)"));
  EXPECT_TRUE(string::contains(out2, "SURVIVED"));
  EXPECT_FALSE(result.getDetected());

  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  fs::remove(mrPath);
}

TEST_F(EvaluatorTest, testEvaluatorWithBuildFailure) {
  Evaluator mEvaluator(ORI_DIR, SAMPLE_BASE);

  captureStdout();
  auto mrPath = OUT_DIR / "newDir" / "MutationResult";
  auto emptyPath = OUT_DIR / "emptyDir";
  fs::create_directories(emptyPath);
  auto result = mEvaluator.compareAndSaveMutationResult(*mutable2, emptyPath, mrPath, "build_failure");
  std::string out2 = capturedStdout();
  EXPECT_TRUE(string::contains(out2, "BOR : "));
  EXPECT_TRUE(string::contains(out2, ".cpp (1:1-1:1 -> |)"));
  EXPECT_TRUE(string::contains(out2, "BUILD_FAILURE"));
  EXPECT_FALSE(result.getDetected());

  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  fs::remove(mrPath);
  fs::remove(emptyPath);
}

TEST_F(EvaluatorTest, testEvaluatorWithRuntimeError) {
  Evaluator mEvaluator(ORI_DIR, SAMPLE_BASE);

  captureStdout();
  auto mrPath = OUT_DIR / "newDir" / "MutationResult";
  auto emptyPath = OUT_DIR / "emptyDir";
  fs::create_directories(emptyPath);
  auto result = mEvaluator.compareAndSaveMutationResult(*mutable2, emptyPath, mrPath, "success");
  std::string out2 = capturedStdout();
  EXPECT_TRUE(string::contains(out2, "BOR : "));
  EXPECT_TRUE(string::contains(out2, ".cpp (1:1-1:1 -> |)"));
  EXPECT_TRUE(string::contains(out2, "RUNTIME_ERROR"));
  EXPECT_FALSE(result.getDetected());

  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  fs::remove(mrPath);
  fs::remove(emptyPath);
}

TEST_F(EvaluatorTest, testEvaluatorWithTimeout) {
  Evaluator mEvaluator(ORI_DIR, SAMPLE_BASE);

  captureStdout();
  auto mrPath = OUT_DIR / "newDir" / "MutationResult";
  auto emptyPath = OUT_DIR / "emptyDir";
  fs::create_directories(emptyPath);
  auto result = mEvaluator.compareAndSaveMutationResult(*mutable2, emptyPath, mrPath, "timeout");
  std::string out2 = capturedStdout();
  EXPECT_TRUE(string::contains(out2, "BOR : "));
  EXPECT_TRUE(string::contains(out2, ".cpp (1:1-1:1 -> |)"));
  EXPECT_TRUE(string::contains(out2, "TIMEOUT"));
  EXPECT_FALSE(result.getDetected());

  MutationResults MRs;
  MRs.load(mrPath);
  auto mr = MRs[0];
  EXPECT_TRUE(mr.compare(result));
  fs::remove(mrPath);
  fs::remove(emptyPath);
}
}  // namespace sentinel
