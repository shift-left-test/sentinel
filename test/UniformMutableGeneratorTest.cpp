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
#include <algorithm>
#include <iostream>
#include "sentinel/UniformMutableGenerator.hpp"


namespace sentinel {

TEST(UniformMutableGeneratorTest,  testAllMutationOperatorsSuccess) {
  SourceLines sourceLines;
  std::string targetFile = "input/sample1/sample1.cpp";
  sourceLines.push_back(SourceLine(targetFile, 58));
  sourceLines.push_back(SourceLine(targetFile, 59));

  UniformMutableGenerator generator{".."};
  Mutables mutables = generator.populate("mutables.db", sourceLines);

  Mutables truth("truth mutables");
  truth.add(Mutable("LCR", targetFile, "sumOfEvenPositiveNumber",
                    58, 29, 58, 31, "||"));
  truth.add(Mutable("LCR", targetFile, "sumOfEvenPositiveNumber",
                    58, 9, 58, 37, "1"));
  truth.add(Mutable("LCR", targetFile, "sumOfEvenPositiveNumber",
                    58, 9, 58, 37, "0"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 17, 58, 19, "!="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 17, 58, 19, "<"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 17, 58, 19, "<="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 17, 58, 19, ">"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 17, 58, 19, ">="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 9, 58, 28, "1"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 9, 58, 28, "0"));
  truth.add(Mutable("BOR", targetFile, "sumOfEvenPositiveNumber",
                    58, 12, 58, 13, "^"));
  truth.add(Mutable("BOR", targetFile, "sumOfEvenPositiveNumber",
                    58, 12, 58, 13, "|"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    58, 10, 58, 11, "((i)++)"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    58, 10, 58, 11, "((i)--)"));
  truth.add(Mutable("SOR", targetFile, "sumOfEvenPositiveNumber",
                    58, 23, 58, 25, ">>"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 34, 58, 35, "!="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 34, 58, 35, "<"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 34, 58, 35, "<="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 34, 58, 35, "=="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 34, 58, 35, ">="));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 32, 58, 37, "1"));
  truth.add(Mutable("ROR", targetFile, "sumOfEvenPositiveNumber",
                    58, 32, 58, 37, "0"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    58, 32, 58, 33, "((i)++)"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    58, 32, 58, 33, "((i)--)"));
  truth.add(Mutable("SDL", targetFile, "sumOfEvenPositiveNumber",
                    59, 7, 59, 20, ""));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    59, 7, 59, 10, "((ret)++)"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    59, 7, 59, 10, "((ret)--)"));
  truth.add(Mutable("AOR", targetFile, "sumOfEvenPositiveNumber",
                    59, 17, 59, 18, "%"));
  truth.add(Mutable("AOR", targetFile, "sumOfEvenPositiveNumber",
                    59, 17, 59, 18, "*"));
  truth.add(Mutable("AOR", targetFile, "sumOfEvenPositiveNumber",
                    59, 17, 59, 18, "-"));
  truth.add(Mutable("AOR", targetFile, "sumOfEvenPositiveNumber",
                    59, 17, 59, 18, "/"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    59, 13, 59, 16, "((ret)++)"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    59, 13, 59, 16, "((ret)--)"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    59, 19, 59, 20, "((i)++)"));
  truth.add(Mutable("UOI", targetFile, "sumOfEvenPositiveNumber",
                    59, 19, 59, 20, "((i)--)"));
  ASSERT_EQ(mutables.size(), truth.size());

  for (const auto& e1 : mutables) {
    bool truthContainE1 = std::any_of(truth.begin(), truth.end(),
        [e1](const auto& e2) { return e2.compare(e1); } );
    EXPECT_TRUE(truthContainE1);
  }
}

}  // namespace sentinel
