/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Mutant.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class MutationResultsTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();

    BASE = fs::temp_directory_path() / "SENTINEL_MUTATIONRESULTSTEST_TMP_DIR";
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
  EXPECT_TRUE(fs::equivalent(MR1.getMutant().getPath(), TARGET_FILE));
  EXPECT_EQ(MR1.getMutant().getFirst().line, 0);
  EXPECT_FALSE(MR1.getDetected());
  EXPECT_EQ("testAdd", MR1.getKillingTest());
}

TEST_F(MutationResultsTest, testGetFailsWhenGivenIndexOutOfRange) {
  MutationResults m;
  EXPECT_THROW(m[0], std::out_of_range);
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

}  // namespace sentinel
