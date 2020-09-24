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

#include <fmt/core.h>
#include <iostream>
#include <map>
#include <args/args.hxx>
#include "sentinel/util/os.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutableGenerator.hpp"
#include "sentinel/UniformMutableSelector.hpp"

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
  args::Positional<std::size_t> count(parser, "COUNT",
    "Max mutable count",
    args::Options::Required);

  parser.Parse();

  sentinel::GitRepository gitRepo(git.Get());
  sentinel::SourceLines sourceLines = gitRepo.getSourceLines();

  std::shared_ptr<sentinel::MutableGenerator> generator =
      std::make_shared<sentinel::UniformMutableGenerator>(input.Get());
  std::shared_ptr<sentinel::MutableSelector> selector =
      std::make_shared<sentinel::UniformMutableSelector>();

  sentinel::MutationFactory mutationFactory(generator, selector);

  sentinel::Mutables mutables = mutationFactory.populate(
      git.Get(),
      sourceLines,
      count.Get());

  mutables.save(sentinel::os::path::join(output.Get(), "mutables.db"));

  std::cout << "MutableCount: " << mutables.size() << std::endl;
}
