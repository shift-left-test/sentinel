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


#include <fmt/core.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <fstream>
#include <string>


namespace sentinel {

class  SampleFileGeneratorForTest : public ::testing::Test {
 protected:
  void SetUp() override {
    namespace fs = std::experimental::filesystem;
    SAMPLE_BASE = fs::temp_directory_path() / "SENTINEL_SAMPLE_DIR";
    fs::remove_all(SAMPLE_BASE);
    SAMPLE1_DIR = SAMPLE_BASE / "sample1";
    fs::create_directories(SAMPLE1_DIR);

    SAMPLE1_NAME = "sample1.cpp";
    SAMPLE1_PATH = SAMPLE1_DIR / SAMPLE1_NAME;

    SAMPLE1B_NAME = "sample1b.cpp";
    SAMPLE1B_PATH = SAMPLE1_DIR / SAMPLE1B_NAME;

    SAMPLECOMCOMJSON_NAME = "compile_commands.json";
    SAMPLECOMCOMJSON_PATH = SAMPLE1_DIR / SAMPLECOMCOMJSON_NAME;

    SAMPLECOVERAGE_NAME = "coverage.info";
    SAMPLECOVERAGE_PATH = SAMPLE1_DIR / SAMPLECOVERAGE_NAME;

    writeFile(SAMPLE1_PATH, SAMPLE1_CONTENTS);
    writeFile(SAMPLE1B_PATH, SAMPLE1B_CONTENTS);
    writeFile(SAMPLECOMCOMJSON_PATH, fmt::format(SAMPLECOMCOMJSON_CONTENTS,
          SAMPLE1_DIR.string()));
    writeFile(SAMPLECOVERAGE_PATH, fmt::format(SAMPLECOVERAGE_CONTENTS,
                                               SAMPLE1_PATH.string()));
  }

  void TearDown() override {
    std::experimental::filesystem::remove_all(SAMPLE_BASE);
  }

  void writeFile(const std::experimental::filesystem::path& p,
      const std::string& c) {
    std::ofstream t(p.string());
    t << c;
    t.close();
  }

  std::experimental::filesystem::path SAMPLE_BASE;
  std::experimental::filesystem::path SAMPLE1_DIR;
  std::experimental::filesystem::path SAMPLE1_PATH;
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

enum Days {
  Sunday, Monday, Tuesday,
  Wednesday, Thursday, Friday, Saturday
};

bool isWeekend(Days d) {
  if (d > Friday) {
    return true;
  }

  return false;
}

// Pointer operation test
int charArraySizie(const char *ptr_start, const char *ptr_end) {
  return (ptr_end - ptr_start) / sizeof(char);
}

int sumOfEvenPositiveNumber(int from, int to) {
  int ret = 0;
  int i = from;

  while (lessThanOrEqual(i, to)) {
    if ((i & 1) == (1 << 0) && i > 0) {
      ret = ret + i;
    }
    i++;
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

void blockUOIInLambdaCapture() {
  int a = 1;
  auto foo = [a] (int x) {
    return x;
  };
}

class Book {
public:
  Book() {}
  int num_pages;
};

Book temporaryBook() {
  return Book();
}

int blockUOIForMaterializedTemporaryExpr() {
  int ret = temporaryBook().num_pages;
  return ret;
})a1s2d3f4";
  std::experimental::filesystem::path SAMPLE1B_PATH;
  std::string SAMPLE1B_NAME;
  std::string SAMPLE1B_CONTENTS =
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

#include <stdexcept>

int sdlBlockedCases() {
  for (;;) break;  // NOLINT
  do {} while (true);
  while (true) {}

  return ({3;});

  int a = 1;
  int* ptr = &a;
  delete ptr;

  try {
    switch (a) {
      case 1:
        break;
      default:
        break;
    }
  } catch (const std::runtime_error& e) {
    return -1;
  }
})a1s2d3f4";
  std::experimental::filesystem::path SAMPLECOMCOMJSON_PATH;
  std::string SAMPLECOMCOMJSON_NAME;
  std::string SAMPLECOMCOMJSON_CONTENTS =
      R"a1b3([
{{
  "directory": "{0}",
  "command": "/usr/bin/c++      -o CMakeFiles/sample1.dir/sample1.cpp.o -c {0}/sample1.cpp",
  "file": "{0}/sample1.cpp"
}},
{{
  "directory": "{0}",
  "command": "/usr/bin/c++      -o CMakeFiles/sample1b.dir/sample1b.cpp.o -c {0}/sample1b.cpp",
  "file": "{0}/sample1b.cpp"
}}
])a1b3";

  std::experimental::filesystem::path SAMPLECOVERAGE_PATH;
  std::string SAMPLECOVERAGE_NAME;
  std::string SAMPLECOVERAGE_CONTENTS =
      R"a1b3(TN:
SF:{0}
FN:33,_ZN18CDiag_Monitor_NodeD0Ev
FN:33,_ZN18CDiag_Monitor_NodeD2Ev
FNDA:0,_ZN18CDiag_Monitor_NodeD0Ev
FNDA:1,_ZN18CDiag_Monitor_NodeD2Ev
FNF:2
FNH:1
DA:33,2
DA:35,1
DA:39,0
DA:40,0
LF:3
LH:1
end_of_record)a1b3";
};

}  // namespace sentinel

#endif  // TEST_INCLUDE_SAMPLEFILEGENERATORFORTEST_HPP_
