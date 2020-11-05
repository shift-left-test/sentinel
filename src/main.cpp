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

#include <experimental/filesystem>
#include <iostream>
#include <memory>
#include <list>
#include <CLI11.hpp>
#include "sentinel/Logger.hpp"
#include "sentinel/CommandPopulate.hpp"
#include "sentinel/CommandMutate.hpp"
#include "sentinel/CommandEvaluate.hpp"
#include "sentinel/CommandReport.hpp"
#include "sentinel/CommandStandAlone.hpp"


int main(int argc, char** argv) {
  namespace fs = std::experimental::filesystem;

  CLI::App app("sentinel");
  app.set_help_all_flag("--help-all", "Expand all help");

  std::list<std::unique_ptr<sentinel::Command>> commandList;

  CLI::Option* verbose = app.add_flag("-v,--verbose", "Verbosity");
  std::string work_dir("./sentinel_tmp");
  app.add_option("-w,--work-dir", work_dir,
    "Sentinel temporary working directory.", true);
  std::string output_dir(".");
  app.add_option("-o,--output-dir", output_dir,
    "Directory for saving output.", true);
  std::string source_root(".");
  app.add_option("source-root", source_root,
    "source root directory.")->required();

  commandList.push_back(std::make_unique<sentinel::CommandPopulate>(&app));
  commandList.push_back(std::make_unique<sentinel::CommandMutate>(&app));
  commandList.push_back(std::make_unique<sentinel::CommandEvaluate>(&app));
  commandList.push_back(std::make_unique<sentinel::CommandReport>(&app));
  commandList.push_back(std::make_unique<sentinel::CommandStandAlone>(&app));

  CLI11_PARSE(app, argc, argv);

  if (verbose != nullptr) {
    sentinel::Logger::setLevel(sentinel::Logger::Level::INFO);
  }

  source_root = fs::canonical(source_root);
  fs::create_directories(work_dir);
  work_dir = fs::canonical(work_dir);
  fs::create_directories(output_dir);
  output_dir = fs::canonical(output_dir);

  for (auto& subcmd : commandList) {
    if (subcmd->isParsed()) {
      return subcmd->run(source_root, work_dir, output_dir, verbose != nullptr);
    }
  }

  std::cout << app.help();

  return -1;
}
