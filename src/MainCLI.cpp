/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <experimental/filesystem>
#include <iostream>
#include <memory>
#include <list>
#include "sentinel/CommandPopulate.hpp"
#include "sentinel/CommandMutate.hpp"
#include "sentinel/CommandEvaluate.hpp"
#include "sentinel/CommandReport.hpp"
#include "sentinel/CommandRun.hpp"
#include "sentinel/CommandGui.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MainCLI.hpp"

namespace sentinel {

int MainCLI(int argc, char** argv) {
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
      mainCommand = std::make_unique<sentinel::CommandRun>(subParser);
      subParser.Parse();
    });
  args::Command gui(commands, "gui",
    "Run sentinel in GUI mode",
    [&](args::Subparser& subParser) {
      mainCommand = std::make_unique<sentinel::CommandGui>(subParser);
      subParser.Parse();
    });

  args::GlobalOptions globals(parser, arguments);

  try {
    parser.helpParams.showTerminator = false;
    parser.helpParams.addDefault = true;

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

}  // namespace sentinel
