/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include <vector>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

class WeightedMutantGeneratorTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    TARGET_FILE1 = SAMPLE1_PATH;
    TARGET_FILE2 = SAMPLE1B_PATH;
    sourceLines = new SourceLines();
    sourceLines->push_back(SourceLine(TARGET_FILE1, 41));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 58));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 59));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 61));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 64));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 68));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 73));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 75));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 76));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 100));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 28));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 29));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 30));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 32));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 36));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 38));
    sourceLines->push_back(SourceLine(TARGET_FILE2, 39));

    allMutants = new Mutants();
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 9, 41, 10, ">="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 9, 41, 10, "<"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 9, 41, 10, "<="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 9, 41, 10, "=="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 9, 41, 10, "!="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 7, 41, 17, "0"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "isWeekend", 41, 7, 41, 17, "1"));
    allMutants->push_back(Mutant("LCR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"));
    allMutants->push_back(Mutant("LCR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 9, 58, 37, "1"));
    allMutants->push_back(Mutant("LCR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 9, 58, 37, "0"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "!="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "<"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "<="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, ">"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, ">="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 9, 58, 28, "1"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 9, 58, 28, "0"));
    allMutants->push_back(Mutant("BOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 12, 58, 13, "^"));
    allMutants->push_back(Mutant("BOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 12, 58, 13, "|"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 10, 58, 11, "(++(i))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 10, 58, 11, "(--(i))"));
    allMutants->push_back(Mutant("SOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 23, 58, 25, ">>"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "!="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "<"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "<="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "=="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, ">="));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 32, 58, 37, "1"));
    allMutants->push_back(Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 32, 58, 37, "0"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 32, 58, 33, "(++(i))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 58, 32, 58, 33, "(--(i))"));
    allMutants->push_back(Mutant("SDL", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 7, 59, 21, "{}"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "%"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "*"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "-"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "/"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 13, 59, 16, "(++(ret))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 13, 59, 16, "(--(ret))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 19, 59, 20, "(++(i))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber", 59, 19, 59, 20, "(--(i))"));
    allMutants->push_back(Mutant("SDL", TARGET_FILE1, "sumOfEvenPositiveNumber", 61, 5, 61, 9, "{}"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "getIntArraySize", 68, 24, 68, 25, "+"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "getIntArraySize", 68, 24, 68, 25, "-"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "getIntArraySize", 68, 24, 68, 25, "*"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "getIntArraySize", 68, 24, 68, 25, "%"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "foo", 75, 7, 75, 8, "(!(b))"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "foo", 76, 18, 76, 19, "-"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "foo", 76, 30, 76, 31, "*"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "foo", 76, 30, 76, 31, "-"));
    allMutants->push_back(Mutant("AOR", TARGET_FILE1, "foo", 76, 30, 76, 31, "/"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "foo", 76, 32, 76, 33, "(++(f))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE1, "foo", 76, 32, 76, 33, "(--(f))"));
    allMutants->push_back(Mutant("SDL", TARGET_FILE2, "sdlBlockedCases", 28, 12, 28, 18, "{}"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE2, "sdlBlockedCases", 39, 13, 39, 14, "(++(a))"));
    allMutants->push_back(Mutant("UOI", TARGET_FILE2, "sdlBlockedCases", 39, 13, 39, 14, "(--(a))"));
  }

  void TearDown() override {
    delete allMutants;
    delete sourceLines;
    SampleFileGeneratorForTest::TearDown();
  }

  Mutants* allMutants = nullptr;
  SourceLines* sourceLines = nullptr;
  std::string TARGET_FILE1;
  std::string TARGET_FILE2;
};

TEST_F(WeightedMutantGeneratorTest, testPopulateFailWhenInvalidDirGiven) {
  WeightedMutantGenerator generator{SAMPLE_BASE};
  EXPECT_THROW(Mutants mutants = generator.populate(*sourceLines, 100), IOException);
}

TEST_F(WeightedMutantGeneratorTest, testPopulateWorkWhenLimitNotExceeded) {
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.populate(*sourceLines, 100);

  std::vector<std::size_t> lines = {41, 58, 59, 61, 68, 75, 76, 28, 39};

  ASSERT_EQ(mutants.size(), 9);
  EXPECT_EQ(mutants[0].getFirst().line, 59);

  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(allMutants->begin(), allMutants->end(), [e1](const auto& e2) { return e2 == e1; }));

    // Check each selected line is unique
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(WeightedMutantGeneratorTest, testPopulateWorkWhenLimitExceeded) {
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.populate(*sourceLines, 4);

  std::vector<std::size_t> lines = {32, 38, 39, 58, 59, 61, 76};

  ASSERT_EQ(mutants.size(), 4);
  EXPECT_EQ(mutants[0].getFirst().line, 59);

  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(allMutants->begin(), allMutants->end(), [e1](const auto& e2) { return e2 == e1; }));

    // Check each selected line is unique
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(WeightedMutantGeneratorTest, testRandomWithDifferentSeedWorks) {
  WeightedMutantGenerator generator1{SAMPLE1_DIR};
  Mutants mutants1 = generator1.populate(*sourceLines, 3, 1);

  WeightedMutantGenerator generator2{SAMPLE1_DIR};
  Mutants mutants2 = generator2.populate(*sourceLines, 3, 3);

  ASSERT_EQ(mutants1.size(), 3);
  ASSERT_EQ(mutants2.size(), 3);
  EXPECT_TRUE(mutants1[0] != mutants2[0] || mutants1[1] != mutants2[1] || mutants1[2] != mutants2[2]);
}

TEST_F(WeightedMutantGeneratorTest, testRandomWithSameSeedWorks) {
  WeightedMutantGenerator generator1{SAMPLE1_DIR};
  Mutants mutants1 = generator1.populate(*sourceLines, 3, 1);

  WeightedMutantGenerator generator2{SAMPLE1_DIR};
  Mutants mutants2 = generator2.populate(*sourceLines, 3, 1);

  ASSERT_EQ(mutants1.size(), 3);
  ASSERT_EQ(mutants2.size(), 3);
  EXPECT_TRUE(mutants1[0] == mutants2[0] && mutants1[1] == mutants2[1] && mutants1[2] == mutants2[2]);
}

}  // namespace sentinel
