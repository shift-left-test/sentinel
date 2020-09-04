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
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(
      "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp", 58));
  sourceLines.push_back(SourceLine(
      "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp", 59));
  sourceLines.push_back(SourceLine(
      "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp", 62));
  Mutables mutables{"somename.db"};
  mutables.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 29, 58, 31, "||"));
  mutables.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 28, "1"));
  mutables.add(Mutable(
      "SOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 21, 58, 27, "1"));
  mutables.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 17, 59, 18, "-"));
  mutables.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 13, 59, 16, "((ret)--)"));

  UniformMutableSelector selector;
  Mutables selected = selector.select(mutables, sourceLines, 3);

  // 1 mutable on line 58, 1 mutable on line 59 are selected
  EXPECT_EQ(selected.size(), 2);
  EXPECT_EQ(selected.get(0).getFirst().line, 58);
  EXPECT_EQ(selected.get(1).getFirst().line, 59);
}

TEST(UniformMutableSelectorTest, testSelectorWorksWhenMaxMutantNumExceeded) {
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(
      "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp", 58));
  sourceLines.push_back(SourceLine(
      "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp", 59));
  sourceLines.push_back(SourceLine(
      "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp", 62));
  Mutables mutables{"somename.db"};
  mutables.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 29, 58, 31, "||"));
  mutables.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 28, "1"));
  mutables.add(Mutable(
      "SOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 21, 58, 27, "1"));
  mutables.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 17, 59, 18, "-"));
  mutables.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 13, 59, 16, "((ret)--)"));

  UniformMutableSelector selector;
  Mutables selected = selector.select(mutables, sourceLines, 1);

  // 1 mutable on line 58, 1 mutable on line 59 are selected
  EXPECT_EQ(selected.size(), 1);
  EXPECT_TRUE(selected.get(0).getFirst().line == 58 ||
              selected.get(0).getFirst().line == 59);
}

}  // namespace sentinel

