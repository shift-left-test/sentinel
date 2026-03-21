/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <iostream>
#include <memory>
#include <string>
#include "sentinel/CliConfigParser.hpp"
#include "sentinel/ConfigResolver.hpp"
#include "sentinel/Console.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/YamlConfigParser.hpp"
#include "sentinel/stages/BaselineBuildStage.hpp"
#include "sentinel/stages/BaselineTestStage.hpp"
#include "sentinel/stages/CheckConfigStage.hpp"
#include "sentinel/stages/DryRunStage.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/stages/InitStage.hpp"
#include "sentinel/stages/PopulateStage.hpp"
#include "sentinel/stages/ReportStage.hpp"
#include "sentinel/version.hpp"

namespace fs = std::filesystem;

/**
 * @brief Application entry point.
 */
int main(int argc, char** argv) {
  args::ArgumentParser parser("A mutation testing tool for C/C++ projects",
                              "For more information, please visit: "
                              "https://github.com/shift-left-test/sentinel");

  args::HelpFlag h(parser, "help", "Display this help menu.", {'h', "help"});
  args::HelpFlag v(parser, "version", "Display the program version.", {'v', "version"});

  sentinel::CliConfigParser cliParser(parser);

  try {
    parser.helpParams.showTerminator = false;
    parser.helpParams.addDefault = true;
    parser.helpParams.width = 120;
    parser.ParseCLI(argc, argv);

    // 1. Get CLI config
    sentinel::Config cliCfg = cliParser.getConfig();

    // 2. Determine workspace path early for mode detection
    fs::path workDirPath = cliCfg.workDir ? fs::absolute(*cliCfg.workDir) : fs::absolute("./.sentinel");
    sentinel::Workspace ws(workDirPath);

    // 3. Detect run mode
    bool dryRun = cliCfg.dryRun;
    bool force  = cliCfg.force && *cliCfg.force;
    bool alreadyComplete = ws.hasPreviousRun() && ws.isComplete() && !force && !dryRun;
    bool resuming = !alreadyComplete && ws.hasPreviousRun() && !force && !dryRun &&
                    sentinel::Console::confirm("Previous run found in '{}'. Resume?",
                                              workDirPath.string());

    // 4. Load and resolve config
    sentinel::Config yamlCfg;
    fs::path configPath = cliParser.getConfigFile();

    if (!cliCfg.init) {
      if (alreadyComplete || resuming) {
        configPath = workDirPath / "config.yaml";
        yamlCfg = sentinel::YamlConfigParser::loadFromFile(configPath);
      } else {
        if (configPath.empty() && fs::exists("sentinel.yaml")) {
          configPath = "sentinel.yaml";
        }
        if (!configPath.empty()) {
          yamlCfg = sentinel::YamlConfigParser::loadFromFile(configPath);
        }
      }
    }

    sentinel::Config cfg = sentinel::ConfigResolver::resolve(cliCfg, yamlCfg, configPath);

    // 5. Configure logger
    auto logger = sentinel::Logger::getLogger("sentinel");
    if (cfg.debug && *cfg.debug) {
      logger->setLevel(sentinel::Logger::Level::DEBUG);
    } else if (cfg.verbose && *cfg.verbose) {
      logger->setLevel(sentinel::Logger::Level::VERBOSE);
    }

    // 6. Create StatusLine
    sentinel::StatusLine statusLine;
    statusLine.setDryRun(dryRun);
    if (!cfg.noStatusLine) {
      statusLine.enable();
    }

    // 7. Initialize workspace for fresh runs
    if (!alreadyComplete && !resuming && !cfg.init) {
      ws.initialize();
    }

    // 8. Assemble stage chain
    auto initStage     = std::make_shared<sentinel::InitStage>(cfg, statusLine, logger);
    auto checkConfig   = std::make_shared<sentinel::CheckConfigStage>(cfg, statusLine, logger, workDirPath);
    auto baselineBuild = std::make_shared<sentinel::BaselineBuildStage>(cfg, statusLine, logger, workDirPath);
    auto baselineTest  = std::make_shared<sentinel::BaselineTestStage>(cfg, statusLine, logger, workDirPath);
    auto populate      = std::make_shared<sentinel::PopulateStage>(cfg, statusLine, logger, workDirPath);
    auto dryRunStage   = std::make_shared<sentinel::DryRunStage>(cfg, statusLine, logger, workDirPath);
    auto evaluation    = std::make_shared<sentinel::EvaluationStage>(cfg, statusLine, logger, workDirPath);
    auto report        = std::make_shared<sentinel::ReportStage>(cfg, statusLine, logger, workDirPath);

    initStage->setNext(checkConfig);
    checkConfig->setNext(baselineBuild);
    baselineBuild->setNext(baselineTest);
    baselineTest->setNext(populate);
    populate->setNext(dryRunStage);
    dryRunStage->setNext(evaluation);
    evaluation->setNext(report);

    // 9. Run chain and return exit code
    return initStage->handle();
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
