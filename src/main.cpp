/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/CliConfigParser.hpp"
#include "sentinel/ConfigResolver.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationRunner.hpp"
#include "sentinel/YamlConfigParser.hpp"
#include "sentinel/version.hpp"

namespace fs = std::filesystem;

/**
 * @brief Application entry point.
 */
int main(int argc, char** argv) {
  args::ArgumentParser parser("A mutation testing tool for C/C++ projects",
                              "For more information, please visit: "
                              "https://github.com/shift-left-test/sentinel");

  // Basic flags
  args::HelpFlag h(parser, "help", "Display this help menu.", {'h', "help"});
  args::HelpFlag v(parser, "version", "Display the program version.", {'v', "version"});

  // 1. Define CLI options
  sentinel::CliConfigParser cliParser(parser);

  try {
    parser.helpParams.showTerminator = false;
    parser.helpParams.addDefault = true;
    parser.helpParams.width = 120;

    // 2. Parse CLI
    parser.ParseCLI(argc, argv);

    // 3. Load YAML if exists or specified (skip if --init)
    sentinel::Config cliCfg = cliParser.getConfig();
    sentinel::Config yamlCfg;
    fs::path configPath = cliParser.getConfigFile();

    if (!cliCfg.init) {
      if (configPath.empty() && fs::exists("sentinel.yaml")) {
        configPath = "sentinel.yaml";
      }

      if (!configPath.empty()) {
        yamlCfg = sentinel::YamlConfigParser::loadFromFile(configPath);
      }
    }

    // 4. Resolve Final Config (Merge CLI > YAML > Default)
    sentinel::Config finalCfg = sentinel::ConfigResolver::resolve(cliCfg, yamlCfg, configPath);

    // 5. Initialize and Run Command
    sentinel::MutationRunner runner(finalCfg);
    runner.init();
    return runner.run();
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
}
