# MIT License
#
# Copyright (c) 2020 Sung Gon Kim
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

if(NOT ENABLE_TEST)
  return()
endif()

find_package(Clang REQUIRED)
link_directories(${LLVM_LIBRARY_DIRS})

build_test_program(
  NAME unittest
  SRCS GitHarnessTest.cpp StringTest.cpp ExceptionTest.cpp
       GitRepositoryTest.cpp ResultTest.cpp MutantsTest.cpp
       LoggerTest.cpp UniformMutantGeneratorTest.cpp MutationResultsTest.cpp
       MutationFactoryTest.cpp EvaluatorTest.cpp
       GitSourceTreeTest.cpp XMLReportTest.cpp HTMLReportTest.cpp ReportTest.cpp
       ContainerTest.cpp RandomMutantGeneratorTest.cpp
       WeightedMutantGeneratorTest.cpp
  PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/test/include
  LIBS sentinel-core git-harness ${LLVM_CLANG_LIBS}
)
