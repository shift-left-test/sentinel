/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

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

#include <iostream>
#include <sstream>
#include <args/args.hxx>
#include "sentinel/Evaluator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutable.hpp"


void evaluateCommand(args::Subparser &parser) {  // NOLINT
  args::ValueFlag<std::string> input(parser, "MUTABLE",
    "Mutable string",
    {'i', "input"}, args::Options::Required);
  args::ValueFlag<std::string> expected(parser, "test_dir",
    "Expected result directory",
    {'e', "expected"}, args::Options::Required);
  args::ValueFlag<std::string> actual(parser, "test_dir",
    "Actual result directory",
    {'a', "actual"}, args::Options::Required);
  args::ValueFlag<std::string> output(parser, "eval_dir",
    "Mutation applied test result",
    {'o', "output"}, ".");

  parser.Parse();

  sentinel::Mutable m;
  std::istringstream iss(input.Get());
  iss >> m;

  sentinel::Evaluator evaluator(m, expected.Get(), output.Get());

  evaluator.compareAndSaveMutationResult(
      actual.Get());
}
