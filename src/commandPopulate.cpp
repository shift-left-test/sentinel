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
#include "sentinel/util/filesystem.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/UniformMutableGenerator.hpp"

void populateCommand(args::Subparser &parser) {  // NOLINT
  args::ValueFlag<std::string> input(parser, "compile_db",
    "Compilation command db path",
    {'i', "input"}, args::Options::Required);
  args::ValueFlag<std::string> git(parser, "git_dir",
    "Git repository dir",
    {'g', "git"}, args::Options::Required);
  args::ValueFlag<std::string> output(parser, "mutable_db",
    "Mutable database output dir",
    {'o', "output"}, ".");
  args::Positional<int> count(parser, "COUNT",
    "Max mutable count",
    args::Options::Required);

  parser.Parse();

  sentinel::GitRepository gitRepo(git.Get());
  sentinel::SourceLines sourceLines = gitRepo.getSourceLines();

  sentinel::UniformMutableGenerator generator(input.Get());
  sentinel::Mutables generated = generator.populate(
    sentinel::util::filesystem::join(output.Get(), "mutables.db"),
    sourceLines);

  std::cout << "MutableCount: " << generated.size() << std::endl;
}
