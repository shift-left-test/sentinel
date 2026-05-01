/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>  // NOLINT
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
    mSourceLines.push_back(SourceLine(mTargetFile1, 100));
    mSourceLines.push_back(SourceLine(mTargetFile2, 28));
    mSourceLines.push_back(SourceLine(mTargetFile2, 29));
    mSourceLines.push_back(SourceLine(mTargetFile2, 30));
    mSourceLines.push_back(SourceLine(mTargetFile2, 33));
    mSourceLines.push_back(SourceLine(mTargetFile2, 37));
    mSourceLines.push_back(SourceLine(mTargetFile2, 39));
    mSourceLines.push_back(SourceLine(mTargetFile2, 40));

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
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 18, 76, 19, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "*"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "-"));
    mAllMutants.push_back(Mutant("AOR", mTargetFile1, "foo", 76, 30, 76, 31, "/"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 76, 32, 76, 33, "(++(f))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile1, "foo", 76, 32, 76, 33, "(--(f))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile2, "sdlBlockedCases", 40, 13, 40, 14, "(++(a))"));
    mAllMutants.push_back(Mutant("UOI", mTargetFile2, "sdlBlockedCases", 40, 13, 40, 14, "(--(a))"));
  }

  Mutants mAllMutants;
  SourceLines mSourceLines;
  std::string mTargetFile1;
  std::string mTargetFile2;
  // cppcheck-suppress unusedStructMember
  static constexpr unsigned kSeed = 1234;
};

TEST_F(WeightedMutantGeneratorTest, testGenerateFailWhenInvalidDirGiven) {
  WeightedMutantGenerator generator{SAMPLE_BASE};
  EXPECT_THROW(Mutants mutants = generator.generate(mSourceLines, 100, kSeed), IOException);
}

TEST_F(WeightedMutantGeneratorTest, testGenerateWorkWhenLimitNotExceeded) {
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 100, kSeed);

  std::vector<std::size_t> lines = {41, 58, 59, 61, 68, 75, 76, 40};

  ASSERT_EQ(mutants.size(), 8);
  EXPECT_EQ(mutants[0].getFirst().line, 59);

  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(mAllMutants.begin(), mAllMutants.end(), [e1](const auto& e2) { return e2 == e1; }));

    // Check each selected line is unique
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(WeightedMutantGeneratorTest, testGenerateWorkWhenLimitExceeded) {
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 4, kSeed);

  std::vector<std::size_t> lines = {33, 39, 40, 58, 59, 61, 76};

  ASSERT_EQ(mutants.size(), 4);
  EXPECT_EQ(mutants[0].getFirst().line, 59);

  for (const auto& e1 : mutants) {
    EXPECT_TRUE(std::any_of(mAllMutants.begin(), mAllMutants.end(), [e1](const auto& e2) { return e2 == e1; }));

    // Check each selected line is unique
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(WeightedMutantGeneratorTest, testRandomWithDifferentSeedWorks) {
  WeightedMutantGenerator generator1{SAMPLE1_DIR};
  Mutants mutants1 = generator1.generate(mSourceLines, 3, kSeed);

  WeightedMutantGenerator generator2{SAMPLE1_DIR};
  Mutants mutants2 = generator2.generate(mSourceLines, 3, kSeed + 1);

  ASSERT_EQ(mutants1.size(), 3);
  ASSERT_EQ(mutants2.size(), 3);
  EXPECT_TRUE(mutants1[0] != mutants2[0] || mutants1[1] != mutants2[1] || mutants1[2] != mutants2[2]);
}

TEST_F(WeightedMutantGeneratorTest, testRandomWithSameSeedWorks) {
  WeightedMutantGenerator baselineGenerator{SAMPLE1_DIR};
  Mutants baseline = baselineGenerator.generate(mSourceLines, 3, kSeed);

  ASSERT_EQ(baseline.size(), 3);
  for (int i = 0; i < 10; ++i) {
    WeightedMutantGenerator generator{SAMPLE1_DIR};
    Mutants mutants = generator.generate(mSourceLines, 3, kSeed);

    ASSERT_EQ(mutants.size(), baseline.size());
    EXPECT_TRUE(std::equal(mutants.begin(), mutants.end(), baseline.begin()));
  }
}

TEST_F(WeightedMutantGeneratorTest, testGetLinesByPathReturnsPerFileLineCounts) {
  namespace fs = std::filesystem;
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  generator.generate(mSourceLines, 100, kSeed);

  const auto& linesByPath = generator.getLinesByPath();
  EXPECT_FALSE(linesByPath.empty());

  // Sum of per-file lines must equal total candidate count
  std::size_t totalLines = 0;
  for (const auto& [path, count] : linesByPath) {
    EXPECT_FALSE(path.empty());
    EXPECT_GT(count, 0u);
    totalLines += count;
  }
  EXPECT_EQ(totalLines, generator.getCandidateCount());

  // Both sample files should be present
  auto canon1 = fs::canonical(SAMPLE1_PATH);
  auto canon1b = fs::canonical(SAMPLE1B_PATH);
  EXPECT_NE(linesByPath.find(canon1), linesByPath.end());
  EXPECT_NE(linesByPath.find(canon1b), linesByPath.end());
}

TEST_F(WeightedMutantGeneratorTest, testGenerateWithMutantsPerLineTwo) {
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 0, kSeed, 2);
  WeightedMutantGenerator generator1{SAMPLE1_DIR};
  Mutants baseline = generator1.generate(mSourceLines, 0, kSeed, 1);
  EXPECT_GT(mutants.size(), baseline.size());
}

TEST_F(WeightedMutantGeneratorTest, testGenerateWithMutantsPerLineUnlimited) {
  WeightedMutantGenerator generator{SAMPLE1_DIR};
  Mutants mutants = generator.generate(mSourceLines, 0, kSeed, 0);
  WeightedMutantGenerator generator1{SAMPLE1_DIR};
  Mutants baseline = generator1.generate(mSourceLines, 0, kSeed, 1);
  EXPECT_GE(mutants.size(), baseline.size());
}

TEST_F(WeightedMutantGeneratorTest, testProgressCallbackInvokedPerFile) {
  WeightedMutantGenerator generator(SAMPLE1_DIR);
  generator.setOperators({"AOR"});

  std::size_t lastDone = 0;
  std::size_t lastTotal = 0;
  std::size_t callCount = 0;
  generator.setProgressCallback(
      [&](std::size_t done, std::size_t total) {
        lastDone = done;
        lastTotal = total;
        ++callCount;
      });

  generator.generate(mSourceLines, /*maxMutants=*/0, /*randomSeed=*/42);

  EXPECT_GE(callCount, 1u);
  EXPECT_EQ(lastDone, lastTotal);
  EXPECT_GT(lastTotal, 0u);
}

}  // namespace sentinel
