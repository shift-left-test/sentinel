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
#include <string>
#include <args/args.hxx>
#include "sentinel/HTMLReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/XMLReport.hpp"


void reportCommand(args::Subparser &parser) {  // NOLINT
  args::ValueFlag<std::string> input(parser, "eval_dir",
    "Mutation test result directory",
    {'i', "input"}, args::Options::Required);
  args::ValueFlag<std::string> output(parser, "report_dir",
    "Mutation test report directory",
    {'o', "output"}, ".");
  args::Positional<std::string> source_root(parser, "source_root",
    "source root directory",
    args::Options::Required);
  args::Flag verbose(parser, "verbose", "Verbosity", {'v', "verbose"});

  parser.Parse();

  if (verbose) {
    sentinel::Logger::setLevel(sentinel::Logger::Level::INFO);
  }

  auto mRPath = sentinel::os::path::join(input.Get(), "MutationResult");
  sentinel::XMLReport xmlReport(mRPath, source_root.Get());
  xmlReport.save(output.Get());
  sentinel::HTMLReport htmlReport(mRPath, source_root.Get());
  htmlReport.save(output.Get());
  htmlReport.printSummary();
}
