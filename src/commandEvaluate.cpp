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
#include <args/args.hxx>


void evaluateCommand(args::Subparser &parser) {  // NOLINT
  args::ValueFlag<std::string> input(parser, "mutable_db",
    "Mutable database dir",
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
  args::Positional<int> index(parser, "INDEX",
    "Index of 'mutable database' to be evaluated",
    args::Options::Required);

  parser.Parse();

  std::cout << "Evaluate"
    << " " << input.Get()
    << " " << expected.Get()
    << " " << actual.Get()
    << " " << output.Get()
    << " " << index.Get()
    << std::endl;
}
