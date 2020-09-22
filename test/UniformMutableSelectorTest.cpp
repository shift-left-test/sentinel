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
#include <iostream>
#include "sentinel/UniformMutableSelector.hpp"


namespace sentinel {

TEST(UniformMutableSelectorTest, testSelectorWorksWhenMaxMutantNumNotExceeded) {
  std::string targetFilename = "input/sample1/sample1.cpp";
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(targetFilename, 58));
  sourceLines.push_back(SourceLine(targetFilename, 59));
  sourceLines.push_back(SourceLine(targetFilename, 62));
  Mutables mutables;
  mutables.push_back(Mutable("LCR", targetFilename, "sumOfEvenPositiveNumber",
                       58, 29, 58, 31, "||"));
  mutables.push_back(Mutable("ROR", targetFilename, "sumOfEvenPositiveNumber",
                       58, 9, 58, 28, "1"));
  mutables.push_back(Mutable("SOR", targetFilename, "sumOfEvenPositiveNumber",
                       58, 21, 58, 27, "1"));
  mutables.push_back(Mutable("AOR", targetFilename, "sumOfEvenPositiveNumber",
                       59, 17, 59, 18, "-"));
  mutables.push_back(Mutable("UOI", targetFilename, "sumOfEvenPositiveNumber",
                       59, 13, 59, 16, "((ret)--)"));

  UniformMutableSelector selector;
  Mutables selected = selector.select(mutables, sourceLines, 3);

  // 1 mutable on line 58, 1 mutable on line 59 are selected
  EXPECT_EQ(selected.size(), 2);
  EXPECT_EQ(selected.at(0).getFirst().line, 58);
  EXPECT_EQ(selected.at(1).getFirst().line, 59);
}

TEST(UniformMutableSelectorTest, testSelectorWorksWhenMaxMutantNumExceeded) {
  std::string targetFilename = "input/sample1/sample1.cpp";
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(targetFilename, 58));
  sourceLines.push_back(SourceLine(targetFilename, 59));
  sourceLines.push_back(SourceLine(targetFilename, 62));
  Mutables mutables;
  mutables.push_back(Mutable("LCR", targetFilename, "sumOfEvenPositiveNumber",
                       58, 29, 58, 31, "||"));
  mutables.push_back(Mutable("ROR", targetFilename, "sumOfEvenPositiveNumber",
                       58, 9, 58, 28, "1"));
  mutables.push_back(Mutable("SOR", targetFilename, "sumOfEvenPositiveNumber",
                       58, 21, 58, 27, "1"));
  mutables.push_back(Mutable("AOR", targetFilename, "sumOfEvenPositiveNumber",
                       59, 17, 59, 18, "-"));
  mutables.push_back(Mutable("UOI", targetFilename, "sumOfEvenPositiveNumber",
                       59, 13, 59, 16, "((ret)--)"));

  UniformMutableSelector selector;
  Mutables selected = selector.select(mutables, sourceLines, 1);

  // 1 mutable on line 58, 1 mutable on line 59 are selected
  EXPECT_EQ(selected.size(), 1);
  EXPECT_TRUE(selected.at(0).getFirst().line == 58 ||
              selected.at(0).getFirst().line == 59);
}

}  // namespace sentinel
