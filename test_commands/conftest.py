import os
import shutil
from subprocess import Popen
import pytest


def pytest_addoption(parser):
    parser.addoption("--executable", help="sentinel binary path", default="./build/src/sentinel")


class SentinelEnv:
    def __init__(self, executable, tmppath):
        self.bin = executable

        self.source_dir = os.path.join(tmppath, "source")
        self.make_sample_dir(self.source_dir)
        self.build_dir = os.path.join(tmppath, "build")
        self.work_dir = os.path.join(tmppath, "work")
        self.output_dir = os.path.join(tmppath, "output")
        self.mutant = "ROR,{}/sample1.cpp,sumOfEvenPositiveNumber,58,17,58,19,<=".format(os.path.abspath(self.source_dir))
        self.expected_dir = os.path.join(tmppath, "expected")
        os.mkdir(self.expected_dir)
        self.actual_dir = os.path.join(tmppath, "actual")
        os.mkdir(self.actual_dir)
        self.eval_file_name = "mutation_result"
        self.eval_file = os.path.join(self.work_dir, self.eval_file_name)

        proc_git = Popen(["git", "init"], cwd=self.source_dir)
        proc_git.wait()
        proc_git = Popen(["git", "add", "."], cwd=self.source_dir)
        proc_git.wait()

        proc_cmake = Popen(["cmake", "-S", self.source_dir, "-B{}".format(self.build_dir)])
        proc_cmake.wait()


    def make_sample_dir(self, target_path):
        files = {}
        files["CMakeLists.txt"] = r"""# cmake_minimum_required(VERSION 3.10)

# set the project name
# project(sample1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add the executable
# add_executable(sample1 sample1.cpp)
add_library(sample1 STATIC sample1.cpp)

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_executable(sample1_test sample1.cpp test_main.cpp)
target_link_libraries(sample1_test gtest gtest_main pthread)

add_test(
    NAME sample1_test
    COMMAND sample1_test
)"""
        files["sample1.cpp"] = r"""/*
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
}"""
        files["test_main.cpp"] = r"""/*
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

#include <gtest/gtest.h>


int sumOfEvenPositiveNumber(int from, int to);

TEST(SampleTest, testSumOfEvenPositiveNumber) {
    EXPECT_EQ(sumOfEvenPositiveNumber(2, 10), 24);
}"""

        os.makedirs(target_path, exist_ok=True)
        for file_name in files:
            with open(os.path.join(target_path, file_name), "w") as cur_file:
                cur_file.write(files[file_name])


@pytest.fixture(scope="session")
def sentinel_env(request, tmpdir_factory):
    tmppath = tmpdir_factory.mktemp("work")

    def cleanup():
        pass
        #shutil.rmtree(tmppath)

    request.addfinalizer(cleanup)
    return SentinelEnv(request.config.getoption("--executable"), tmppath)
