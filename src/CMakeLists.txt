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

find_package(Clang REQUIRED)

build_static_library(
  NAME sentinel-core
  SRCS SourceLine.cpp SourceTree.cpp Mutable.cpp
       UniformMutableGenerator.cpp UniformMutableSelector.cpp
       MutationFactory.cpp GitRepository.cpp Result.cpp Evaluator.cpp
       MutationResult.cpp GitSourceTree.cpp
       Report.cpp XMLReport.cpp HTMLReport.cpp
       Logger.cpp operators/aor.cpp operators/bor.cpp operators/lcr.cpp
       operators/ror.cpp operators/sdl.cpp operators/sor.cpp operators/uoi.cpp
       operators/MutationOperator.cpp
  PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include
  PUBLIC_SYSTEM_HEADERS ${CLANG_INCLUDE_DIRS}
  LIBS tinyxml2 fmt git2
  NO_INSTALL
)

build_program(
  NAME sentinel
  SRCS main.cpp
       commandPopulate.cpp
       commandMutate.cpp
       commandEvaluate.cpp
       commandReport.cpp
  LIBS sentinel-core args ${LLVM_CLANG_LIBS}
)
