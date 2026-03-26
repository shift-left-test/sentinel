/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <csignal>
#include <filesystem>  // NOLINT
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/CliConfigParser.hpp"
#include "sentinel/ConfigResolver.hpp"
#include "sentinel/ConfigValidator.hpp"
#include "sentinel/Console.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/SignalHandler.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/YamlConfigParser.hpp"
#include "sentinel/YamlConfigWriter.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"
#include "sentinel/stages/OriginalBuildStage.hpp"
#include "sentinel/stages/OriginalTestStage.hpp"
#include "sentinel/stages/DryRunStage.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/stages/GenerationStage.hpp"
#include "sentinel/stages/ReportStage.hpp"
#include "sentinel/version.hpp"

namespace fs = std::filesystem;

/**
 * @brief Runs the sentinel application after CLI arguments have been parsed.
 *
 * @param cliParser Populated CLI config parser (non-const: args library flags require mutable access).
 * @return Exit code (0 on success).
 */
static int runApplication(sentinel::CliConfigParser* cliParser) {
  // 1. Get CLI config
  sentinel::Config cliCfg = cliParser->getConfig();

  // Handle --init: write config template and exit (before workspace creation)
  if (cliCfg.init) {
    const char* const kConfigFileName = "sentinel.yaml";
    if (fs::exists(kConfigFileName) && !(cliCfg.force && *cliCfg.force)) {
      if (!sentinel::Console::confirm("'{}' already exists and will be overwritten.", kConfigFileName)) {
        return 0;
      }
    }
    sentinel::YamlConfigWriter::writeTemplate(kConfigFileName);
    return 0;
  }

  // 2. Determine workspace path early for mode detection
  fs::path workDirPath = cliCfg.workDir ? fs::absolute(*cliCfg.workDir) : fs::absolute("./.sentinel");
  auto ws = std::make_shared<sentinel::Workspace>(workDirPath);

  // 3. Detect run mode
  bool dryRun = cliCfg.dryRun;
  bool force = cliCfg.force && *cliCfg.force;
  bool alreadyComplete = false;
  bool resuming = false;
  if (!force && !dryRun && ws->hasPreviousRun()) {
    if (!sentinel::Console::confirm("A previous run was found in '{}' and will be resumed.", workDirPath)) {
      return 0;
    }
    alreadyComplete = ws->isComplete();
    resuming = !alreadyComplete;
  }

  // 4. Load and resolve config
  sentinel::Config yamlCfg;
  fs::path configPath = cliParser->getConfigFile();

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

  sentinel::Config cfg = sentinel::ConfigResolver::resolve(cliCfg, yamlCfg, configPath);

  // 5. Configure logger
  if (cfg.verbose && *cfg.verbose) {
    sentinel::Logger::setLevel(sentinel::Logger::Level::VERBOSE);
  }
  if (dryRun && ws->hasPreviousRun()) {
    sentinel::Logger::info("Workspace '{}' exists and will be cleared for dry-run.", workDirPath);
  }

  // Validate config before starting the pipeline.
  // Skipped for resumed/already-complete runs: the config was already validated at the start
  // of the original run and is reloaded from workspace (config.yaml).
  if (!alreadyComplete && !resuming) {
    if (!sentinel::ConfigValidator::validate(cfg)) {
      return 0;  // user declined warning prompt
    }
  }

  // 6. Create StatusLine
  auto statusLine = std::make_shared<sentinel::StatusLine>();
  statusLine->setDryRun(dryRun);
  if (!cfg.noStatusLine) {
    statusLine->enable();
  }

  // 7. Initialize workspace for fresh runs
  if (!alreadyComplete && !resuming) {
    ws->initialize();
  }

  // 8. Assemble stage chain
  auto originalBuild = std::make_shared<sentinel::OriginalBuildStage>(cfg, statusLine, ws);
  auto originalTest = std::make_shared<sentinel::OriginalTestStage>(cfg, statusLine, ws);
  auto generation = std::make_shared<sentinel::GenerationStage>(cfg, statusLine, ws);
  auto dryRunStage = std::make_shared<sentinel::DryRunStage>(cfg, statusLine, ws);
  auto evaluation = std::make_shared<sentinel::EvaluationStage>(cfg, statusLine, ws);
  auto report = std::make_shared<sentinel::ReportStage>(cfg, statusLine, ws);

  originalBuild->setNext(originalTest)->setNext(generation)->setNext(dryRunStage)->setNext(evaluation)->setNext(report);

  // 9. Install signal handlers before pipeline starts
  const std::vector<int> signals = {SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1};
  sentinel::SignalHandler::add(signals, [ws, &cfg]() { ws->restoreBackup(*cfg.sourceDir); });
  sentinel::SignalHandler::add(signals, [statusLine]() { statusLine->disable(); });

  // 10. Run pipeline
  originalBuild->run();
  return 0;
}

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
  } catch (const args::Help& e) {
    if (std::strcmp(e.what(), "version") == 0) {
      std::cout << "sentinel " << PROGRAM_VERSION << std::endl;
    } else {
      std::cout << parser;
    }
    return 0;
  } catch (const args::Error& e) {
    sentinel::Logger::error("{}", e.what());
    std::cerr << parser;
    return 2;
  }

  try {
    return runApplication(&cliParser);
  } catch (const sentinel::ThresholdError& e) {
    sentinel::Logger::error("{}", e.what());
    return 3;
  } catch (const std::exception& e) {
    sentinel::Logger::error("{}", e.what());
    return 1;
  }
}
