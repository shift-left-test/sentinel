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
#include "sentinel/Logger.hpp"
#include "sentinel/CommandPopulate.hpp"
#include "sentinel/CommandMutate.hpp"
#include "sentinel/CommandEvaluate.hpp"
#include "sentinel/CommandReport.hpp"
#include "sentinel/CommandStandAlone.hpp"


int main(int argc, char** argv) {
  std::unique_ptr<sentinel::Command> mainCommand;
  args::Group arguments("arguments");
  args::HelpFlag h(arguments, "help",
    "Display this help menu. \n"
    "Use 'sentinal COMMAND --help' to see help for each command.",
    {'h', "help"});

  args::ArgumentParser parser("Mutation Test");

  args::Group commands(static_cast<args::Group&>(parser), "commands");
  args::Command populate(commands, "populate",
    "Identify mutable test targets and application methods in'git' "
    "and print a list",
    [&](args::Subparser& subParser) {
      mainCommand = std::make_unique<sentinel::CommandPopulate>(subParser);
      subParser.Parse();
    });
  args::Command mutate(commands, "mutate",
    "Apply the selected 'mutable' to the source. "
    "The original file is backed up in 'work-dir'",
    [&](args::Subparser& subParser) {
      mainCommand = std::make_unique<sentinel::CommandMutate>(subParser);
      subParser.Parse();
    });
  args::Command evaluate(commands, "evaluate",
    "Compare the test result with mutable applied and the test result "
    "not applied",
    [&](args::Subparser& subParser) {
      mainCommand = std::make_unique<sentinel::CommandEvaluate>(subParser);
      subParser.Parse();
    });
  args::Command report(commands, "report",
    "Create a mutation test report based on the'evaluate' result "
    "and source code",
    [&](args::Subparser& subParser) {
      mainCommand = std::make_unique<sentinel::CommandReport>(subParser);
      subParser.Parse();
    });
  args::Command run(commands, "run",
    "Run mutation test in standalone mode",
    [&](args::Subparser& subParser) {
      mainCommand = std::make_unique<sentinel::CommandStandAlone>(subParser);
      subParser.Parse();
    });

  args::GlobalOptions globals(parser, arguments);

  try {
    parser.ParseCLI(argc, argv);

    if (mainCommand) {
      mainCommand->init();
      return mainCommand->run();
    }
  } catch (args::Help& e) {
    std::cout << parser;
  } catch (args::Error& e) {
    std::cerr << e.what() << std::endl << parser;
    return 1;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 2;
  }

  return -1;
}
