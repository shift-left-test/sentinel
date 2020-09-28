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
#include "sentinel/UniformMutableGenerator.hpp"


namespace sentinel {

class UniformMutableGeneratorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TARGET_FILE = "input/sample1/sample1.cpp";
    sourceLines = new SourceLines();
    sourceLines->push_back(SourceLine(TARGET_FILE, 58));
    sourceLines->push_back(SourceLine(TARGET_FILE, 59));
    sourceLines->push_back(SourceLine(TARGET_FILE, 67));
    sourceLines->push_back(SourceLine(TARGET_FILE, 72));
    sourceLines->push_back(SourceLine(TARGET_FILE, 74));
    sourceLines->push_back(SourceLine(TARGET_FILE, 75));
    sourceLines->push_back(SourceLine(TARGET_FILE, 82));
    sourceLines->push_back(SourceLine(TARGET_FILE, 83));
    sourceLines->push_back(SourceLine(TARGET_FILE, 84));
    sourceLines->push_back(SourceLine(TARGET_FILE, 86));

    allMutables = new Mutables();
    allMutables->push_back(
        Mutable("LCR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 29, 58, 31, "||"));
    allMutables->push_back(
        Mutable("LCR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 9, 58, 37, "1"));
    allMutables->push_back(
        Mutable("LCR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 9, 58, 37, "0"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, "!="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, "<"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, "<="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, ">"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 17, 58, 19, ">="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 9, 58, 28, "1"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 9, 58, 28, "0"));
    allMutables->push_back(
        Mutable("BOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 12, 58, 13, "^"));
    allMutables->push_back(
        Mutable("BOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 12, 58, 13, "|"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 10, 58, 11, "((i)++)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 10, 58, 11, "((i)--)"));
    allMutables->push_back(
        Mutable("SOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 23, 58, 25, ">>"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "!="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "<"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "<="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, "=="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 34, 58, 35, ">="));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 32, 58, 37, "1"));
    allMutables->push_back(
        Mutable("ROR", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 32, 58, 37, "0"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 32, 58, 33, "((i)++)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                58, 32, 58, 33, "((i)--)"));
    allMutables->push_back(
        Mutable("SDL", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 7, 59, 20, ""));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "%"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "*"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "-"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 17, 59, 18, "/"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 13, 59, 16, "((ret)++)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 13, 59, 16, "((ret)--)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 19, 59, 20, "((i)++)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "sumOfEvenPositiveNumber",
                59, 19, 59, 20, "((i)--)"));
    allMutables->push_back(
        Mutable("SDL", TARGET_FILE, "getIntArraySize",
                67, 3, 67, 37, ""));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "getIntArraySize",
                67, 24, 67, 25, "+"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "getIntArraySize",
                67, 24, 67, 25, "-"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "getIntArraySize",
                67, 24, 67, 25, "*"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "getIntArraySize",
                67, 24, 67, 25, "%"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "foo",
                74, 7, 74, 8, "(!(b))"));
    allMutables->push_back(
        Mutable("SDL", TARGET_FILE, "foo",
                75, 5, 75, 35, ""));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "foo",
                75, 12, 75, 35, "((*(ptr + int(VAR_I + f)))++)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "foo",
                75, 12, 75, 35, "((*(ptr + int(VAR_I + f)))--)"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "foo",
                75, 18, 75, 19, "-"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "foo",
                75, 30, 75, 31, "*"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "foo",
                75, 30, 75, 31, "-"));
    allMutables->push_back(
        Mutable("AOR", TARGET_FILE, "foo",
                75, 30, 75, 31, "/"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "foo",
                75, 32, 75, 33, "((f)++)"));
    allMutables->push_back(
        Mutable("UOI", TARGET_FILE, "foo",
                75, 32, 75, 33, "((f)--)"));
    allMutables->push_back(
        Mutable("SDL", TARGET_FILE, "sdlBlockedCases",
                82, 12, 82, 17, ""));
    allMutables->push_back(
        Mutable("SDL", TARGET_FILE, "sdlBlockedCases",
                86, 3, 86, 16, ""));
  }

  void TearDown() override {
    delete allMutables;
    delete sourceLines;
  }

  Mutables* allMutables = nullptr;
  SourceLines* sourceLines = nullptr;
  std::string TARGET_FILE;
};

TEST_F(UniformMutableGeneratorTest, testPopulateWorkWhenLimitNotExceeded) {
  UniformMutableGenerator generator{".."};
  Mutables mutables = generator.populate(*sourceLines, 100);

  std::vector<std::size_t> lines = {58, 59, 67, 72, 74, 75, 82, 83, 84, 86};

  ASSERT_EQ(mutables.size(), 7);
  for (const auto& e1 : mutables) {
    EXPECT_TRUE(std::any_of(allMutables->begin(), allMutables->end(),
        [e1](const auto& e2) { return e2.compare(e1); }));

    // Check each selected line is unique
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

TEST_F(UniformMutableGeneratorTest, testPopulateWorkWhenLimitExceeded) {
  UniformMutableGenerator generator{".."};
  Mutables mutables = generator.populate(*sourceLines, 3);

  std::vector<std::size_t> lines = {58, 59, 67, 72, 74, 75, 82, 83, 84, 86};

  ASSERT_EQ(mutables.size(), 3);
  for (const auto& e1 : mutables) {
    EXPECT_TRUE(std::any_of(allMutables->begin(), allMutables->end(),
        [e1](const auto& e2) { return e2.compare(e1); }));

    // Check each selected line is unique
    std::size_t lineNum = e1.getFirst().line;
    auto pos = std::find(lines.begin(), lines.end(), lineNum);
    EXPECT_NE(pos, lines.end());
    lines.erase(pos);
  }
}

}  // namespace sentinel
