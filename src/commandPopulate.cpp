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

#include <map>
#include <args/args.hxx>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutableGenerator.hpp"


void populateCommand(args::Subparser &parser) {  // NOLINT
  args::ValueFlag<std::string> compile_db_path(parser, "compile_db_path",
    "Directory where compile_commands.json file exists.",
    {'b', "build"}, args::Options::Required);
  args::ValueFlag<std::string> scope(parser, "scope",
    "diff scope, one of ['commit', 'all']. default: all",
    {'s', "scope"}, "all");
  args::ValueFlagList<std::string> extensions(parser, "extensions",
    "Extentions of source file which could be mutated.",
    {'t', "extensions"},
    {"cxx", "hxx", "cpp", "hpp", "cc", "hh", "c", "h", "c++", "h++",
     "cu", "cuh"});
  args::ValueFlagList<std::string> exclude(parser, "exclude",
    "exclude file or path",
    {'e', "exclude"});
  args::ValueFlag<int> limit(parser, "limit",
    "Maximum generated mutable count. default: 10",
    {'l', "limit"}, 10);
  args::ValueFlag<std::string> output(parser, "mutable_db",
    "Mutable database output filename. default: ./mutables.db",
    {'o', "output"}, "./mutables.db");
  args::Positional<std::string> source_root(parser, "source_root",
    "source root directory",
    args::Options::Required);
  args::Flag verbose(parser, "verbose", "Verbosity", {'v', "verbose"});

  parser.Parse();

  if (verbose) {
    sentinel::Logger::setLevel(sentinel::Logger::Level::INFO);
  }

  auto repo = std::make_unique<sentinel::GitRepository>(source_root.Get(),
                                                        extensions.Get(),
                                                        exclude.Get());

  sentinel::SourceLines sourceLines = repo->getSourceLines(scope.Get());

  auto generator = std::make_shared<sentinel::UniformMutableGenerator>(
      compile_db_path.Get());

  sentinel::MutationFactory mutationFactory(generator);

  auto mutables = mutationFactory.populate(source_root.Get(),
                                           sourceLines,
                                           limit.Get());

  mutables.save(output.Get());
}
