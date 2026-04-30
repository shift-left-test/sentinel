/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class MutationResultsTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();

    BASE = testTempDir("SENTINEL_MUTATIONRESULTSTEST_TMP_DIR");
    fs::remove_all(BASE);
    OUT_DIR = BASE / "ORI_DIR";
    fs::create_directories(OUT_DIR);
    TARGET_FILE = SAMPLE1_PATH;
  }

  void TearDown() override {
    fs::remove_all(BASE);
    SampleFileGeneratorForTest::TearDown();
  }

  fs::path BASE;
  fs::path OUT_DIR;
  std::string TARGET_FILE;
};

TEST_F(MutationResultsTest, testAdd) {
  MutationResults MRs;
  Mutant M1("AOR", TARGET_FILE, "sumOfEvenPositiveNumber", 0, 0, 0, 0, "+");
  EXPECT_EQ(0, MRs.size());
  MutationResult MR1(M1, "testAdd", "testMinus", MutationState::RUNTIME_ERROR);
  MRs.push_back(MR1);
  EXPECT_EQ(1, MRs.size());

  EXPECT_TRUE(MRs[0].compare(MR1));
  EXPECT_EQ(MRs[0].getMutant().getOperator(), "AOR");
  EXPECT_EQ(MR1.getMutant().getPath(), fs::path(TARGET_FILE));
  EXPECT_EQ(MR1.getMutant().getFirst().line, 0);
  EXPECT_FALSE(MR1.getDetected());
  EXPECT_EQ("testAdd", MR1.getKillingTest());
}

TEST_F(MutationResultsTest, testGetFailsWhenGivenIndexOutOfRange) {
  MutationResults m;
  EXPECT_THROW(m.at(0), std::out_of_range);
}

TEST_F(MutationResultsTest, testSaveAndLoad) {
  MutationResults MRs;

  Mutant M1("AOR", TARGET_FILE, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MutationResult MR1(M1, "", "", MutationState::SURVIVED);
  MRs.push_back(MR1);

  Mutant M2("BOR", TARGET_FILE, "sumOfEvenPositiveNumber", 1, 2, 3, 4, "|");
  MutationResult MR2(M2, "testAddBit", "testAdd", MutationState::RUNTIME_ERROR);
  MRs.push_back(MR2);

  auto mrPath = OUT_DIR / "MutationResult";
  MRs.save(mrPath);
  EXPECT_TRUE(fs::exists(mrPath));

  MutationResults MRs2;
  MRs2.load(mrPath);
  EXPECT_TRUE(MRs2[0].compare(MR1));
  EXPECT_TRUE(MRs2[1].compare(MR2));
}

TEST_F(MutationResultsTest, testGetMutationStateReturnsAllStates) {
  Mutant M("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  EXPECT_EQ(MutationState::KILLED, MutationResult(M, "t1", "", MutationState::KILLED).getMutationState());
  EXPECT_EQ(MutationState::SURVIVED, MutationResult(M, "", "", MutationState::SURVIVED).getMutationState());
  EXPECT_EQ(MutationState::RUNTIME_ERROR, MutationResult(M, "", "t2", MutationState::RUNTIME_ERROR).getMutationState());
  EXPECT_EQ(MutationState::BUILD_FAILURE, MutationResult(M, "", "", MutationState::BUILD_FAILURE).getMutationState());
  EXPECT_EQ(MutationState::TIMEOUT, MutationResult(M, "", "", MutationState::TIMEOUT).getMutationState());
}

TEST_F(MutationResultsTest, testGetDetectedReturnsTrueOnlyForKilled) {
  Mutant M("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  EXPECT_TRUE(MutationResult(M, "t1", "", MutationState::KILLED).getDetected());
  EXPECT_FALSE(MutationResult(M, "", "", MutationState::SURVIVED).getDetected());
  EXPECT_FALSE(MutationResult(M, "", "t2", MutationState::RUNTIME_ERROR).getDetected());
  EXPECT_FALSE(MutationResult(M, "", "", MutationState::BUILD_FAILURE).getDetected());
  EXPECT_FALSE(MutationResult(M, "", "", MutationState::TIMEOUT).getDetected());
}

TEST_F(MutationResultsTest, testGetKillingTestAndErrorTest) {
  Mutant M("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr(M, "killerTest", "errorTest", MutationState::RUNTIME_ERROR);
  EXPECT_EQ("killerTest", mr.getKillingTest());
  EXPECT_EQ("errorTest", mr.getErrorTest());
}

TEST_F(MutationResultsTest, testStreamOperatorYamlRoundTrip) {
  Mutant m("AOR", TARGET_FILE, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MutationResult original(m, "testAdd", "testMinus", MutationState::RUNTIME_ERROR);
  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  MutationResult loaded;
  in >> loaded;
  EXPECT_TRUE(loaded.compare(original));
}

TEST_F(MutationResultsTest, testStreamOperatorYamlRoundTripEmptyFields) {
  Mutant m("SDL", TARGET_FILE, "func", 1, 0, 1, 5, "");
  MutationResult original(m, "", "", MutationState::SURVIVED);
  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  MutationResult loaded;
  in >> loaded;
  EXPECT_TRUE(loaded.compare(original));
}

TEST_F(MutationResultsTest, testStreamOperatorYamlFormat) {
  Mutant m("ROR", TARGET_FILE, "foo", 1, 2, 3, 4, "!=");
  MutationResult mr(m, "testKill", "testError", MutationState::KILLED);
  std::ostringstream out;
  out << mr;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("state: KILLED"));
  EXPECT_NE(std::string::npos, yaml.find("killing-test: testKill"));
  EXPECT_NE(std::string::npos, yaml.find("error-test: testError"));
  EXPECT_NE(std::string::npos, yaml.find("mutant:"));
}

TEST_F(MutationResultsTest, testStreamOperatorEmptyInputSetsFail) {
  std::istringstream in("");
  MutationResult mr;
  in >> mr;
  EXPECT_TRUE(in.fail());
}

TEST_F(MutationResultsTest, testStreamOperatorInvalidYamlSetsFail) {
  std::istringstream in("{{{invalid");
  MutationResult mr;
  in >> mr;
  EXPECT_TRUE(in.fail());
}

TEST_F(MutationResultsTest, testStreamOperatorNonMapYamlSetsFail) {
  std::istringstream in("- just a list\n- not a map\n");
  MutationResult mr;
  in >> mr;
  EXPECT_TRUE(in.fail());
}

TEST_F(MutationResultsTest, testStreamOperatorInvalidStateSetsFail) {
  static constexpr const char* kYaml =
      "state: INVALID_STATE\n"
      "killing-test: \"\"\n"
      "error-test: \"\"\n"
      "mutant:\n"
      "  op: AOR\n"
      "  path: foo.cpp\n"
      "  func: f\n"
      "  first-line: 1\n"
      "  first-col: 1\n"
      "  last-line: 1\n"
      "  last-col: 5\n"
      "  token: \"+\"\n";
  std::istringstream in(kYaml);
  MutationResult mr;
  in >> mr;
  EXPECT_TRUE(in.fail());
}

TEST_F(MutationResultsTest, testDefaultTimingIsZero) {
  MutationResult mr;
  EXPECT_DOUBLE_EQ(0.0, mr.getBuildSecs());
  EXPECT_DOUBLE_EQ(0.0, mr.getTestSecs());

  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr2(m, "t1", "", MutationState::KILLED);
  EXPECT_DOUBLE_EQ(0.0, mr2.getBuildSecs());
  EXPECT_DOUBLE_EQ(0.0, mr2.getTestSecs());
}

TEST_F(MutationResultsTest, testSetAndGetTiming) {
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr(m, "t1", "", MutationState::KILLED);
  mr.setBuildSecs(3.42);
  mr.setTestSecs(12.87);
  EXPECT_DOUBLE_EQ(3.42, mr.getBuildSecs());
  EXPECT_DOUBLE_EQ(12.87, mr.getTestSecs());
}

TEST_F(MutationResultsTest, testStreamOperatorYamlRoundTripWithTiming) {
  Mutant m("AOR", TARGET_FILE, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MutationResult original(m, "testAdd", "testMinus", MutationState::RUNTIME_ERROR);
  original.setBuildSecs(3.42);
  original.setTestSecs(12.87);
  std::ostringstream out;
  out << original;
  std::string yaml = out.str();
  EXPECT_NE(std::string::npos, yaml.find("build-time: 3.42"));
  EXPECT_NE(std::string::npos, yaml.find("test-time: 12.87"));
  std::istringstream in(yaml);
  MutationResult loaded;
  in >> loaded;
  EXPECT_TRUE(loaded.compare(original));
  EXPECT_DOUBLE_EQ(3.42, loaded.getBuildSecs());
  EXPECT_DOUBLE_EQ(12.87, loaded.getTestSecs());
}

TEST_F(MutationResultsTest, testStreamOperatorYamlBackwardCompatibility) {
  static constexpr const char* kYaml =
      "state: KILLED\n"
      "killing-test: testKill\n"
      "error-test: \"\"\n"
      "mutant:\n"
      "  operator: AOR\n"
      "  path: foo.cpp\n"
      "  function: f\n"
      "  first: {line: 1, column: 0}\n"
      "  last: {line: 1, column: 5}\n"
      "  token: \"+\"\n";
  std::istringstream in(kYaml);
  MutationResult mr;
  in >> mr;
  EXPECT_FALSE(in.fail());
  EXPECT_EQ(MutationState::KILLED, mr.getMutationState());
  EXPECT_DOUBLE_EQ(0.0, mr.getBuildSecs());
  EXPECT_DOUBLE_EQ(0.0, mr.getTestSecs());
}

TEST_F(MutationResultsTest, testCompareIncludesTiming) {
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr1(m, "t1", "", MutationState::KILLED);
  MutationResult mr2(m, "t1", "", MutationState::KILLED);
  EXPECT_TRUE(mr1.compare(mr2));

  mr1.setBuildSecs(1.0);
  EXPECT_FALSE(mr1.compare(mr2));

  mr2.setBuildSecs(1.0);
  EXPECT_TRUE(mr1.compare(mr2));

  mr1.setTestSecs(2.0);
  EXPECT_FALSE(mr1.compare(mr2));

  mr2.setTestSecs(2.0);
  EXPECT_TRUE(mr1.compare(mr2));
}

TEST_F(MutationResultsTest, testStreamOperatorAllStatesRoundTrip) {
  const std::vector<MutationState> states = {
      MutationState::KILLED,
      MutationState::SURVIVED,
      MutationState::RUNTIME_ERROR,
      MutationState::BUILD_FAILURE,
      MutationState::TIMEOUT,
  };
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  for (const auto& state : states) {
    MutationResult original(m, "t1", "t2", state);
    std::ostringstream out;
    out << original;
    std::istringstream in(out.str());
    MutationResult loaded;
    in >> loaded;
    EXPECT_FALSE(in.fail());
    EXPECT_EQ(state, loaded.getMutationState());
  }
}

TEST_F(MutationResultsTest, testStreamOperatorMissingMutantFieldSetsFail) {
  static constexpr const char* kYaml =
      "state: KILLED\n"
      "killing-test: test1\n"
      "error-test: \"\"\n"
      "mutant:\n"
      "  operator: AOR\n"
      "  path: foo.cpp\n";
  std::istringstream in(kYaml);
  MutationResult mr;
  in >> mr;
  EXPECT_TRUE(in.fail());
}

TEST_F(MutationResultsTest, testCompareDifferentStatesReturnsFalse) {
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr1(m, "t1", "", MutationState::KILLED);
  MutationResult mr2(m, "t1", "", MutationState::SURVIVED);
  EXPECT_FALSE(mr1.compare(mr2));
}

TEST_F(MutationResultsTest, testCompareDifferentKillingTestReturnsFalse) {
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr1(m, "testA", "", MutationState::KILLED);
  MutationResult mr2(m, "testB", "", MutationState::KILLED);
  EXPECT_FALSE(mr1.compare(mr2));
}

TEST_F(MutationResultsTest, testCompareDifferentErrorTestReturnsFalse) {
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MutationResult mr1(m, "", "errA", MutationState::RUNTIME_ERROR);
  MutationResult mr2(m, "", "errB", MutationState::RUNTIME_ERROR);
  EXPECT_FALSE(mr1.compare(mr2));
}

TEST_F(MutationResultsTest, testCompareDifferentMutantsReturnsFalse) {
  Mutant m1("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  Mutant m2("BOR", TARGET_FILE, "foo", 1, 0, 1, 5, "|");
  MutationResult mr1(m1, "", "", MutationState::SURVIVED);
  MutationResult mr2(m2, "", "", MutationState::SURVIVED);
  EXPECT_FALSE(mr1.compare(mr2));
}

TEST_F(MutationResultsTest, testEmplaceBackCreatesInPlace) {
  MutationResults MRs;
  Mutant m("AOR", TARGET_FILE, "foo", 1, 0, 1, 5, "+");
  MRs.emplace_back(m, "test", "", MutationState::KILLED);
  ASSERT_EQ(1u, MRs.size());
  EXPECT_EQ(MutationState::KILLED, MRs[0].getMutationState());
  EXPECT_EQ("test", MRs[0].getKillingTest());
}

TEST_F(MutationResultsTest, uncoveredFlagDefaultIsFalse) {
  MutationResult result;
  EXPECT_FALSE(result.isUncovered());
}

TEST_F(MutationResultsTest, uncoveredFlagPersistsThroughSerialization) {
  Mutant m("AOR", TARGET_FILE, "sumOfEvenPositiveNumber", 4, 5, 6, 7, "+");
  MutationResult original(m, "", "", MutationState::SURVIVED);
  original.setUncovered(true);

  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());

  MutationResult restored;
  in >> restored;

  EXPECT_TRUE(restored.isUncovered());
  EXPECT_EQ(restored.getMutationState(), MutationState::SURVIVED);
}

TEST_F(MutationResultsTest, uncoveredFlagAbsentInLegacyFormatReadsAsFalse) {
  static constexpr const char* kLegacyYaml =
      "state: SURVIVED\n"
      "killing-test: \"\"\n"
      "error-test: \"\"\n"
      "build-time: 0\n"
      "test-time: 0\n"
      "mutant:\n"
      "  operator: AOR\n"
      "  path: foo.cpp\n"
      "  function: f\n"
      "  first: {line: 1, column: 1}\n"
      "  last: {line: 1, column: 2}\n"
      "  token: \"+\"\n";
  std::istringstream in(kLegacyYaml);
  MutationResult result;
  in >> result;
  EXPECT_FALSE(in.fail());
  EXPECT_FALSE(result.isUncovered());
}

}  // namespace sentinel
