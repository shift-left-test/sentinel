/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

class RandomMutantGeneratorTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    mTargetFile1 = SAMPLE1_PATH;
    mSourceLines.push_back(SourceLine(mTargetFile1, 41));
    mSourceLines.push_back(SourceLine(mTargetFile1, 58));
    mSourceLines.push_back(SourceLine(mTargetFile1, 59));
    mSourceLines.push_back(SourceLine(mTargetFile1, 61));
    mSourceLines.push_back(SourceLine(mTargetFile1, 64));
    mSourceLines.push_back(SourceLine(mTargetFile1, 68));
    mSourceLines.push_back(SourceLine(mTargetFile1, 73));
    mSourceLines.push_back(SourceLine(mTargetFile1, 75));
    mSourceLines.push_back(SourceLine(mTargetFile1, 76));
    mSourceLines.push_back(SourceLine(mTargetFile1, 100));

    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, ">="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "<"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "<="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 9, 41, 10, "=="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 7, 41, 17, "0"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "isWeekend", 41, 7, 41, 17, "1"));
    mAllMutants.push_back(Mutant("LCR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 29, 58, 31, "||"));
    mAllMutants.push_back(Mutant("LCR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 37, "1"));
    mAllMutants.push_back(Mutant("LCR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 37, "0"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "!="));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, "<"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 17, 58, 19, ">"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 28, "1"));
    mAllMutants.push_back(Mutant("ROR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 9, 58, 28, "0"));
    mAllMutants.push_back(Mutant("BOR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 12, 58, 13, "^"));
    mAllMutants.push_back(Mutant("BOR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 12, 58, 13, "|"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 58, 10, 58, 11, "(++(i))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "sumOfEvenPositiveNumber", 58, 10, 58, 11, "(--(i))"));
    mAllMutants.push_back(Mutant("SOR", mTargetFile1, "sumOfEvenPositiveNumber", 58, 23, 58, 25, ">>"));
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
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 18, 76, 19, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "*"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "/"));
  }

  Mutants mAllMutants;
  SourceLines mSourceLines;
  std::string mTargetFile1;
  static constexpr unsigned kSeed = 1234;
};

TEST_F(RandomMutantGeneratorTest, testGenerateFailWhenInvalidDirGiven) {
  RandomMutantGenerator generator{SAMPLE_BASE};
  EXPECT_THROW(Mutants mutants = generator.generate(mSourceLines, 100, kSeed), IOException);
}

TEST_F(RandomMutantGeneratorTest, testGenerateWorkWhenLimitNotExceeded) {
  RandomMutantGenerator generator{SAMPLE1_DIR};
  // source line당 1개이므로 결과는 sourceLines 크기 이하
  Mutants mutants = generator.generate(mSourceLines, 1000, kSeed);

  EXPECT_LE(mutants.size(), mSourceLines.size());
  EXPECT_GT(mutants.size(), 0u);
  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(mAllMutants.begin(), mAllMutants.end(), [e1](const auto& e2) { return e2 == e1; }));
  }
}

TEST_F(RandomMutantGeneratorTest, testGenerateWorkWhenLimitExceeded) {
  RandomMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 3, kSeed);

  ASSERT_EQ(mutants.size(), 3);
  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(mAllMutants.begin(), mAllMutants.end(), [e1](const auto& e2) { return e2 == e1; }));
  }
}

TEST_F(RandomMutantGeneratorTest, testRandomWithDifferentSeedWorks) {
  RandomMutantGenerator generator1{SAMPLE1_DIR};
  Mutants mutants1 = generator1.generate(mSourceLines, 3, kSeed);

  RandomMutantGenerator generator2{SAMPLE1_DIR};
  Mutants mutants2 = generator2.generate(mSourceLines, 3, kSeed + 1);

  ASSERT_EQ(mutants1.size(), 3);
  ASSERT_EQ(mutants2.size(), 3);
  EXPECT_TRUE(mutants1[0] != mutants2[0] || mutants1[1] != mutants2[1] || mutants1[2] != mutants2[2]);
}

TEST_F(RandomMutantGeneratorTest, testRandomWithSameSeedWorks) {
  RandomMutantGenerator baselineGenerator{SAMPLE1_DIR};
  Mutants baseline = baselineGenerator.generate(mSourceLines, 3, kSeed);

  ASSERT_EQ(baseline.size(), 3);
  for (int i = 0; i < 10; ++i) {
    RandomMutantGenerator generator{SAMPLE1_DIR};
    Mutants mutants = generator.generate(mSourceLines, 3, kSeed);

    ASSERT_EQ(mutants.size(), baseline.size());
    EXPECT_TRUE(std::equal(mutants.begin(), mutants.end(), baseline.begin()));
  }
}

TEST_F(RandomMutantGeneratorTest, testGenerateWithZeroLimitReturnsAllCandidates) {
  RandomMutantGenerator generator1{SAMPLE1_DIR};
  Mutants unlimited = generator1.generate(mSourceLines, 0, kSeed);

  RandomMutantGenerator generator2{SAMPLE1_DIR};
  Mutants large = generator2.generate(mSourceLines, 1000, kSeed);

  EXPECT_EQ(unlimited.size(), large.size());
  EXPECT_GT(unlimited.size(), 0u);
}

TEST_F(RandomMutantGeneratorTest, testGetLinesByPathReturnsPerFileLineCounts) {
  namespace fs = std::filesystem;
  RandomMutantGenerator generator{SAMPLE1_DIR};
  generator.generate(mSourceLines, 100, kSeed);

  const auto& linesByPath = generator.getLinesByPath();
  EXPECT_FALSE(linesByPath.empty());

  for (const auto& [path, count] : linesByPath) {
    EXPECT_GT(count, 0u);
  }

  // Only one file in this test fixture
  auto canon1 = fs::canonical(SAMPLE1_PATH);
  EXPECT_NE(linesByPath.find(canon1), linesByPath.end());
}

}  // namespace sentinel
