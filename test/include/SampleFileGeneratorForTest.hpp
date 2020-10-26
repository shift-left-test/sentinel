/*
  MIT License

  Copyright (c) 2020 Sangmo Kang

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef TEST_INCLUDE_SAMPLEFILEGENERATORFORTEST_HPP_
#define TEST_INCLUDE_SAMPLEFILEGENERATORFORTEST_HPP_

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "sentinel/util/os.hpp"


namespace sentinel {

class  SampleFileGeneratorForTest : public ::testing::Test {
 protected:
  void SetUp() override {
    SAMPLE_BASE = os::tempDirectory("input");
    SAMPLE1_DIR = os::tempDirectory(os::path::join(SAMPLE_BASE, "sample1"));
    SAMPLE1_NAME = "sample1.cpp";
    SAMPLE1_PATH = os::path::join(SAMPLE1_DIR, SAMPLE1_NAME);
    std::ofstream t(SAMPLE1_PATH);
    t << SAMPLE1_CONTENTS;
    t.close();
  }

  void TearDown() override {
    os::removeDirectories(SAMPLE_BASE);
  }

  std::string SAMPLE_BASE;
  std::string SAMPLE1_PATH;
  std::string SAMPLE1_DIR;
  std::string SAMPLE1_NAME;
  std::string SAMPLE1_CONTENTS =
      R"a1s2d3f4(/*
  MIT License

  Copyright (c) 2020 Loc Duy Phan

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// #include <iostream>

// Macro test: with and without semicolon (;)
#define IS_NEGATIVE(a) a < 0
#define INVALID_RECT_SIDE_ERR -1;

inline bool lessThanOrEqual(int a, int b) {
  return a <= b;
}

// Return surface area of a rectangular
int rectangularSurfaceArea(const int h, const float l, double w) {
  // LCR ROR UOI test
  // SBR test: if, return
  if (IS_NEGATIVE(h) || IS_NEGATIVE(l) || w < 0)
    return INVALID_RECT_SIDE_ERR

  // Comment test: middle of code
  // AOR test: basic mutation + modulo mutation test
  // UOI test: no const variable mutation
  return 2 * (l * h + w * h/*+ l * l*/+ w * l);
}

// Pointer operation test
int charArraySizie(const char *ptr_start, const char *ptr_end) {
  return (ptr_end - ptr_start) / sizeof(char);
}

int sumOfEvenPositiveNumber(int from, int to) {
  int ret = 0;
  int i = from;

  for (; lessThanOrEqual(i, to); ++i) {
    if ((i & 1) == (1 << 0) && i > 0) {
      ret = ret + i;
    }
  }

  return ret;
}

int getIntArraySize(const int* start, const int* end) {
  return (end - start) / sizeof(int);
}

#define VAR_I i
int foo(int i, float f) {
  int* ptr = &i;
  bool b = i > 0;
  if (b) {
    return *(ptr + int(VAR_I + f));  // NOLINT
  }

  return 0;
}

int sdlBlockedCases() {
  for (;;) break;  // NOLINT
  do {} while (true);
  while (true) {}

  return ({3;});
})a1s2d3f4";
};

}  // namespace sentinel

#endif  // TEST_INCLUDE_SAMPLEFILEGENERATORFORTEST_HPP_
