/*
  MIT License

  Copyright (c) 2020 Loc Duy Phan

  Permission is hereby granted,  free of charge,  to any person obtaining a copy
  of this software and associated documentation files (the "Software"),  to deal
  in the Software without restriction,  including without limitation the rights
  to use,  copy,  modify,  merge,  publish,  distribute,  sublicense,  and/or sell
  copies of the Software,  and to permit persons to whom the Software is
  furnished to do so,  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER
  LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <gtest/gtest.h>
#include <algorithm>
#include "SampleFileGeneratorForTest.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/RandomMutantGenerator.hpp"

namespace sentinel {

class RandomMutantGeneratorTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    TARGET_FILE1 = SAMPLE1_PATH;
    sourceLines = new SourceLines();
    sourceLines->push_back(SourceLine(TARGET_FILE1, 58));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 59));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 61));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 64));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 68));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 73));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 75));
    sourceLines->push_back(SourceLine(TARGET_FILE1, 76));

    allMutants = new Mutants();
    allMutants->push_back(
        Mutant("LCR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 29, 58, 31, "||"));
    allMutants->push_back(
        Mutant("LCR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 9, 58, 37, "1"));
    allMutants->push_back(
        Mutant("LCR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 9, 58, 37, "0"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, "!="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, "<"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, "<="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, ">"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, ">="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 9, 58, 28, "1"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 9, 58, 28, "0"));
    allMutants->push_back(
        Mutant("BOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 12, 58, 13, "^"));
    allMutants->push_back(
        Mutant("BOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 12, 58, 13, "|"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 10, 58, 11, "((i)++)"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 10, 58, 11, "((i)--)"));
    allMutants->push_back(
        Mutant("SOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 23, 58, 25, ">>"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "!="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "<"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "<="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "=="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, ">="));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 32, 58, 37, "1"));
    allMutants->push_back(
        Mutant("ROR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 32, 58, 37, "0"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 32, 58, 33, "((i)++)"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                58, 32, 58, 33, "((i)--)"));
    allMutants->push_back(
        Mutant("SDL", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 7, 59, 20, ""));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "%"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "*"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "-"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "/"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 13, 59, 16, "((ret)++)"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 13, 59, 16, "((ret)--)"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 19, 59, 20, "((i)++)"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "sumOfEvenPositiveNumber",
                59, 19, 59, 20, "((i)--)"));
    allMutants->push_back(
        Mutant("SDL", TARGET_FILE1, "sumOfEvenPositiveNumber",
                61, 5, 61, 8, ""));
    allMutants->push_back(
        Mutant("SDL", TARGET_FILE1, "sumOfEvenPositiveNumber",
                64, 3, 64, 13, ""));
    allMutants->push_back(
        Mutant("SDL", TARGET_FILE1, "getIntArraySize",
                68, 3, 68, 37, ""));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "getIntArraySize",
                68, 24, 68, 25, "+"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "getIntArraySize",
                68, 24, 68, 25, "-"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "getIntArraySize",
                68, 24, 68, 25, "*"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "getIntArraySize",
                68, 24, 68, 25, "%"));
    allMutants->push_back(
        Mutant("UOI", TARGET_FILE1, "foo",
                75, 7, 75, 8, "(!(b))"));
    allMutants->push_back(
        Mutant("SDL", TARGET_FILE1, "foo",
                76, 5, 76, 35, ""));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "foo",
                76, 18, 76, 19, "-"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "foo",
                76, 30, 76, 31, "*"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "foo",
                76, 30, 76, 31, "-"));
    allMutants->push_back(
        Mutant("AOR", TARGET_FILE1, "foo",
                76, 30, 76, 31, "/"));
  }

  void TearDown() override {
    delete allMutants;
    delete sourceLines;
    SampleFileGeneratorForTest::TearDown();
  }

  Mutants* allMutants = nullptr;
  SourceLines* sourceLines = nullptr;
  std::string TARGET_FILE1;
};

TEST_F(RandomMutantGeneratorTest, testPopulateFailWhenInvalidDirGiven) {
  RandomMutantGenerator generator{"."};
  EXPECT_THROW(Mutants mutables = generator.populate(*sourceLines, 100),
               IOException);
}

TEST_F(RandomMutantGeneratorTest, testPopulateWorkWhenLimitNotExceeded) {
  RandomMutantGenerator generator{".."};
  int maxMutants = allMutants->size();
  Mutants mutables = generator.populate(*sourceLines, maxMutants);
  // ASSERT_EQ(mutables.size(), maxMutants);

  for (const auto& e1 : mutables) {
    EXPECT_TRUE(std::any_of(allMutants->begin(), allMutants->end(),
        [e1](const auto& e2) { return e2.compare(e1); }));
  }
}

TEST_F(RandomMutantGeneratorTest, testPopulateWorkWhenLimitExceeded) {
  RandomMutantGenerator generator{".."};
  Mutants mutables = generator.populate(*sourceLines, 3);

  ASSERT_EQ(mutables.size(), 3);
  for (const auto& e1 : mutables) {
    EXPECT_TRUE(std::any_of(allMutants->begin(), allMutants->end(),
        [e1](const auto& e2) { return e2.compare(e1); }));
  }
}

}  // namespace sentinel