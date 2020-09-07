/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

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
#include "sentinel/UniformMutableGenerator.hpp"


namespace sentinel {

TEST(UniformMutableGeneratorTest,  testAllMutationOperatorsSuccess) {
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine("input/sample1/sample1.cpp", 58));
  sourceLines.push_back(SourceLine("input/sample1/sample1.cpp", 59));

  UniformMutableGenerator generator{"/home/jenkins/workspace/sentinel"};
  Mutables mutables = generator.populate("mutables.db", sourceLines);

  /* for (const auto& m : mutables) {
    std::cout << m.getOperator() << ", "
              << m.getPath() << ", "
              << m.getFirst().line << ", "
              << m.getFirst().column << ", "
              << m.getLast().line << ", "
              << m.getLast().column << ", "
              << m.getToken() << std::endl;
  } */

  Mutables truth("truth mutables");
  truth.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 29, 58, 31, "||"));
  truth.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 37, "1"));
  truth.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 37, "0"));
  truth.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 37, "(i & 1) == (1 << 0)"));
  truth.add(Mutable(
      "LCR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 37, "i > 0"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 17, 58, 19, "!="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 17, 58, 19, "<"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 17, 58, 19, "<="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 17, 58, 19, ">"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 17, 58, 19, ">="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 28, "1"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 9, 58, 28, "0"));
  truth.add(Mutable(
      "BOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 12, 58, 13, "^"));
  truth.add(Mutable(
      "BOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 12, 58, 13, "|"));
  truth.add(Mutable(
      "BOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 10, 58, 15, "i"));
  truth.add(Mutable(
      "BOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 10, 58, 15, "1"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 10, 58, 11, "((i)++)"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 10, 58, 11, "((i)--)"));
  truth.add(Mutable(
      "SOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 23, 58, 25, ">>"));
  truth.add(Mutable(
      "SOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 21, 58, 27, "1"));
  truth.add(Mutable(
      "SOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 21, 58, 27, "0"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 34, 58, 35, "!="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 34, 58, 35, "<"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 34, 58, 35, "<="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 34, 58, 35, "=="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 34, 58, 35, ">="));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 32, 58, 37, "1"));
  truth.add(Mutable(
      "ROR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 32, 58, 37, "0"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 32, 58, 33, "((i)++)"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      58, 32, 58, 33, "((i)--)"));
  truth.add(Mutable(
      "SDL", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 7, 59, 20, ""));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 7, 59, 10, "((ret)++)"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 7, 59, 10, "((ret)--)"));
  truth.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 17, 59, 18, "%"));
  truth.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 17, 59, 18, "*"));
  truth.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 17, 59, 18, "-"));
  truth.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 17, 59, 18, "/"));
  truth.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 13, 59, 20, "ret"));
  truth.add(Mutable(
      "AOR", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 13, 59, 20, "i"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 13, 59, 16, "((ret)++)"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 13, 59, 16, "((ret)--)"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 19, 59, 20, "((i)++)"));
  truth.add(Mutable(
      "UOI", "/home/jenkins/workspace/sentinel/test/input/sample1/sample1.cpp",
      59, 19, 59, 20, "((i)--)"));

  ASSERT_EQ(mutables.size(), truth.size());

  for (const auto& e1 : mutables) {
    bool truthContainE1 = false;
    for (const auto& e2 : truth) {
      if (e2.compare(e1)) {
        truthContainE1 = true;
        break;
      }
    }

    EXPECT_TRUE(truthContainE1);
  }
}

}  // namespace sentinel

