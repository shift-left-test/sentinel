/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include <vector>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

class UniformMutantGeneratorTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    mTargetFile1 = SAMPLE1_PATH;
    mTargetFile2 = SAMPLE1B_PATH;
    mSourceLines.push_back(SourceLine(mTargetFile1, 41));
    mSourceLines.push_back(SourceLine(mTargetFile1, 58));
    mSourceLines.push_back(SourceLine(mTargetFile1, 59));
    mSourceLines.push_back(SourceLine(mTargetFile1, 61));
    mSourceLines.push_back(SourceLine(mTargetFile1, 64));
    mSourceLines.push_back(SourceLine(mTargetFile1, 68));
    mSourceLines.push_back(SourceLine(mTargetFile1, 73));
    mSourceLines.push_back(SourceLine(mTargetFile1, 75));
    mSourceLines.push_back(SourceLine(mTargetFile1, 76));
    mSourceLines.push_back(SourceLine(mTargetFile1, 84));
    mSourceLines.push_back(SourceLine(mTargetFile1, 100));
    mSourceLines.push_back(SourceLine(mTargetFile2, 28));
    mSourceLines.push_back(SourceLine(mTargetFile2, 29));
    mSourceLines.push_back(SourceLine(mTargetFile2, 30));
    mSourceLines.push_back(SourceLine(mTargetFile2, 32));
    mSourceLines.push_back(SourceLine(mTargetFile2, 36));
    mSourceLines.push_back(SourceLine(mTargetFile2, 38));
    mSourceLines.push_back(SourceLine(mTargetFile2, 39));

    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, ">="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "<"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "<="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "=="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "!="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 7, 41, 17, "0"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 7, 41, 17, "1"));
    mAllMutants.push_back(Mutant("LCR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"));
    mAllMutants.push_back(Mutant("LCR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 37, "1"));
    mAllMutants.push_back(Mutant("LCR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 37, "0"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "!="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "<"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "<="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, ">"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, ">="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 28, "1"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 28, "0"));
    mAllMutants.push_back(Mutant("BOR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 12, 58, 13, "^"));
    mAllMutants.push_back(Mutant("BOR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 12, 58, 13, "|"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 58, 10, 58, 11, "(++(i))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 58, 10, 58, 11, "(--(i))"));
    mAllMutants.push_back(Mutant("SOR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 23, 58, 25, ">>"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "!="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "<"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "<="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, "=="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 34, 58, 35, ">="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 32, 58, 37, "1"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 32, 58, 37, "0"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 58, 32, 58, 33, "(++(i))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 58, 32, 58, 33, "(--(i))"));
    mAllMutants.push_back(Mutant("SDL", mTargetFile1, "sumOfEvenPositiveNumber", 59, 7, 59, 21, "{}"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "%"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "*"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "sumOfEvenPositiveNumber", 59, 17, 59, 18, "/"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 59, 13, 59, 16, "(++(ret))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 59, 13, 59, 16, "(--(ret))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 59, 19, 59, 20, "(++(i))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 59, 19, 59, 20, "(--(i))"));
    mAllMutants.push_back(Mutant("SDL", mTargetFile1, "sumOfEvenPositiveNumber", 61, 5, 61, 9, "{}"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "getIntArraySize", 68, 24, 68, 25, "+"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "getIntArraySize", 68, 24, 68, 25, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "getIntArraySize", 68, 24, 68, 25, "*"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "getIntArraySize", 68, 24, 68, 25, "%"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 75, 7, 75, 8, "(!(b))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 76, 12, 76, 35, "(++(*(ptr + int(VAR_I + f))))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 76, 12, 76, 35, "(--(*(ptr + int(VAR_I + f))))"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 18, 76, 19, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "*"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "/"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 76, 32, 76, 33, "(++(f))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 76, 32, 76, 33, "(--(f))"));
    mAllMutants.push_back(Mutant("SDL", mTargetFile2, "sdlBlockedCases", 28, 12, 28, 18, "{}"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile2, "sdlBlockedCases", 39, 13, 39, 14, "(++(a))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile2, "sdlBlockedCases", 39, 13, 39, 14, "(--(a))"));
  }

  Mutants mAllMutants;
  SourceLines mSourceLines;
  std::string mTargetFile1;
  std::string mTargetFile2;
  static constexpr unsigned kSeed = 1234;
};

TEST_F(UniformMutantGeneratorTest, testGenerateFailWhenInvalidDirGiven) {
  UniformMutantGenerator generator{SAMPLE_BASE};
  EXPECT_THROW(Mutants mutants = generator.generate(mSourceLines, 100, kSeed), IOException);
}

TEST_F(UniformMutantGeneratorTest, testGenerateWorkWhenLimitNotExceeded) {
  UniformMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 100, kSeed);

  std::vector<std::size_t> lines = {41, 58, 59, 61, 68, 75, 76, 28, 39};

  ASSERT_EQ(mutants.size(), 9);
  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(mAllMutants.begin(), mAllMutants.end(), [e1](const auto& e2) { return e2 == e1; }));

    // Check each selected line is unique.
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(UniformMutantGeneratorTest, testGenerateWorkWhenLimitExceeded) {
  UniformMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 3, kSeed);

  std::vector<std::size_t> lines = {41, 58, 59, 61, 68, 75, 76, 28, 39};

  ASSERT_EQ(mutants.size(), 3);
  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(mAllMutants.begin(), mAllMutants.end(), [e1](const auto& e2) { return e2 == e1; }));

    // Check each selected line is unique.
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(UniformMutantGeneratorTest, testRandomWithDifferentSeedWorks) {
  UniformMutantGenerator generator1{SAMPLE1_DIR};
  Mutants mutants1 = generator1.generate(mSourceLines, 3, kSeed);

  UniformMutantGenerator generator2{SAMPLE1_DIR};
  Mutants mutants2 = generator2.generate(mSourceLines, 3, kSeed + 1);

  ASSERT_EQ(mutants1.size(), 3);
  ASSERT_EQ(mutants2.size(), 3);
  EXPECT_TRUE(mutants1[0] != mutants2[0] || mutants1[1] != mutants2[1] || mutants1[2] != mutants2[2]);
}

TEST_F(UniformMutantGeneratorTest, testRandomWithSameSeedWorks) {
  UniformMutantGenerator generator1{SAMPLE1_DIR};
  Mutants mutants1 = generator1.generate(mSourceLines, 3, kSeed);

  UniformMutantGenerator generator2{SAMPLE1_DIR};
  Mutants mutants2 = generator2.generate(mSourceLines, 3, kSeed);

  ASSERT_EQ(mutants1.size(), 3);
  ASSERT_EQ(mutants2.size(), 3);
  EXPECT_TRUE(mutants1[0] == mutants2[0] && mutants1[1] == mutants2[1] && mutants1[2] == mutants2[2]);
}

TEST_F(UniformMutantGeneratorTest, testGenerateWithZeroLimitReturnsAllCandidates) {
  UniformMutantGenerator generator1{SAMPLE1_DIR};
  Mutants unlimited = generator1.generate(mSourceLines, 0, kSeed);

  UniformMutantGenerator generator2{SAMPLE1_DIR};
  Mutants limited = generator2.generate(mSourceLines, 1000, kSeed);

  // limit=0은 limit=큰수와 동일한 결과여야 함
  EXPECT_EQ(unlimited.size(), limited.size());
  EXPECT_GT(unlimited.size(), 0u);
}

}  // namespace sentinel
