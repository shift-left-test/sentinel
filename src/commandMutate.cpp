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
#include "sentinel/GitRepository.hpp"
#include "sentinel/Mutables.hpp"
#include "sentinel/SourceTree.hpp"
#include "sentinel/util/os.hpp"


void mutateCommand(args::Subparser &parser) {  // NOLINT
  args::ValueFlag<std::string> input(parser, "mutable_db",
    "Mutable database dir",
    {'i', "input"}, args::Options::Required);
  args::ValueFlag<std::string> git(parser, "git_dir",
    "Git repository dir",
    {'g', "git"}, args::Options::Required);
  args::ValueFlag<std::string> backup(parser, "backup_dir",
    "Mutated souce backup dir",
    {'b', "backup"}, args::Options::Required);
  args::Positional<int> index(parser, "INDEX",
    "Index of 'Mutable database' to be mutated",
    args::Options::Required);

  parser.Parse();

  sentinel::Mutables mutables;
  mutables.load(sentinel::os::path::join(input.Get(), "mutables.db"));
  sentinel::Mutable m = mutables.at(index.Get());

  sentinel::GitRepository repository(git.Get());
  repository.getSourceTree()->modify(m, backup.Get());
}
