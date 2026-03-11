/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <iostream>
#include <memory>
#include "sentinel/CommandRun.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MainCLI.hpp"
#include "sentinel/version.hpp"

namespace sentinel {

int MainCLI(int argc, char** argv) {
  args::ArgumentParser parser("A mutation testing tool for C/C++ projects",
                              "For more information, please visit: "
                              "https://github.com/shift-left-test/sentinel");

  args::HelpFlag h(parser, "help", "Display this help menu.", {'h', "help"});
  args::HelpFlag v(parser, "version", "Display the program version.", {'v', "version"});

  sentinel::CommandRun mainCommand(parser);

  try {
    parser.helpParams.showTerminator = false;
    parser.helpParams.addDefault = true;
    parser.helpParams.width = 120;

    parser.ParseCLI(argc, argv);

    mainCommand.init();
    return mainCommand.run();
  } catch (args::Help& e) {
    if (std::strcmp(e.what(), "version") == 0) {
      std::cout << "sentinel " << PROGRAM_VERSION << std::endl;
    } else {
      std::cout << parser;
    }
    return 0;
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
