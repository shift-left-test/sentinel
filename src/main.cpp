/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <csignal>
#include <filesystem>  // NOLINT
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/CliConfigParser.hpp"
#include "sentinel/ConfigValidator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/PartitionedWorkspaceMerger.hpp"
#include "sentinel/PipelineContext.hpp"
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
  // 1. Handle --init
  if (cliParser->isInit()) {
    const char* const kConfigFileName = "sentinel.yaml";
    if (fs::exists(kConfigFileName) && !cliParser->isForce()) {
      throw std::runtime_error(
          fmt::format("'{}' already exists. Use --init --force to overwrite.", kConfigFileName));
    }
    sentinel::YamlConfigWriter::writeTemplate(kConfigFileName);
    return 0;
  }

  // 2. Determine workspace path early
  fs::path workDirPath = cliParser->getWorkDir().empty()
      ? fs::absolute(".sentinel") : cliParser->getWorkDir();

  // 3. Handle --merge-partition (merge mode)
  {
    sentinel::Config mergeCfg = sentinel::Config::withDefaults();
    cliParser->applyTo(&mergeCfg);
    if (!mergeCfg.mergeWorkspaces.empty()) {
      if (mergeCfg.verbose) {
        sentinel::Logger::setLevel(sentinel::Logger::Level::VERBOSE);
      }
      sentinel::Logger::info("Merging partitions into '{}'...", workDirPath);
      sentinel::PartitionedWorkspaceMerger merger(
          workDirPath, mergeCfg.mergeWorkspaces, mergeCfg.force);
      merger.merge();
      return 0;
    }
  }

  auto ws = std::make_shared<sentinel::Workspace>(workDirPath);

  // 4. Detect run mode
  bool alreadyComplete = false;
  bool resuming = false;
  if (!cliParser->isClean() && !cliParser->isDryRun() && ws->hasPreviousRun()) {
    alreadyComplete = ws->isComplete();
    resuming = !alreadyComplete;
    if (resuming) {
      sentinel::Logger::info("Resuming previous run from '{}'.", workDirPath);
    } else {
      sentinel::Logger::info("Previous run is complete. Regenerating report from '{}'.", workDirPath);
    }
  }

  // 5. Build config: defaults -> YAML -> CLI
  sentinel::Config cfg = sentinel::Config::withDefaults();

  if (alreadyComplete || resuming) {
    sentinel::YamlConfigParser::applyTo(&cfg, workDirPath / "config.yaml");
    auto ignored = cliParser->getEffectiveCliOptions();
    if (!ignored.empty()) {
      sentinel::Logger::warn("The following options have no effect when resuming: {}",
                             fmt::join(ignored, ", "));
      sentinel::Logger::warn("Use --clean to start a fresh run with new settings.");
    }
    cliParser->applyReportOnlyTo(&cfg);
  } else {
    fs::path configPath = cliParser->getConfigFile();
    if (configPath.empty() && fs::exists("sentinel.yaml")) {
      configPath = "sentinel.yaml";
    }
    if (!configPath.empty()) {
      sentinel::YamlConfigParser::applyTo(&cfg, configPath);
    }
    cliParser->applyTo(&cfg);
  }

  // 6. Configure logger
  if (cfg.verbose) {
    sentinel::Logger::setLevel(sentinel::Logger::Level::VERBOSE);
  }
  if ((cfg.dryRun || cfg.clean) && ws->hasPreviousRun()) {
    sentinel::Logger::warn("Workspace '{}' exists and will be cleared.", workDirPath);
  }

  sentinel::ConfigValidator::validate(cfg);

  // 7. Create StatusLine
  auto statusLine = std::make_shared<sentinel::StatusLine>();
  statusLine->setDryRun(cfg.dryRun);
  statusLine->enable();

  // 8. Initialize workspace for fresh runs
  if (!alreadyComplete && !resuming) {
    sentinel::Logger::info("Initializing workspace '{}'...", workDirPath);
    ws->initialize();
    sentinel::WorkspaceStatus versionStatus;
    versionStatus.version = PROGRAM_VERSION;
    ws->saveStatus(versionStatus);
  }

  // 9. Create stage-specific dependencies
  auto repo = std::make_shared<sentinel::GitRepository>(cfg.sourceDir, cfg.extensions, cfg.patterns);
  if (cfg.from) {
    repo->validateRevision(*cfg.from);
  }
  auto generator = sentinel::MutantGenerator::getInstance(cfg.generator, cfg.compileDbDir);

  // 10. Assemble stage chain
  auto originalBuild = std::make_shared<sentinel::OriginalBuildStage>();
  auto originalTest = std::make_shared<sentinel::OriginalTestStage>();
  auto generation = std::make_shared<sentinel::GenerationStage>(repo, generator);
  auto dryRunStage = std::make_shared<sentinel::DryRunStage>();
  auto evaluation = std::make_shared<sentinel::EvaluationStage>(repo);
  auto report = std::make_shared<sentinel::ReportStage>();

  originalBuild->setNext(originalTest)->setNext(generation)->setNext(dryRunStage)->setNext(evaluation)->setNext(report);

  // 11. Install signal handlers
  const std::vector<int> signals = {SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1};
  sentinel::SignalHandler::add(signals, [ws, &cfg]() { ws->restoreBackup(cfg.sourceDir); });
  sentinel::SignalHandler::add(signals, [statusLine]() { statusLine->disable(); });

  // 12. Create pipeline context and run
  sentinel::PipelineContext ctx{cfg, *statusLine, *ws};
  originalBuild->run(&ctx);
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
  args::HelpFlag v(parser, "version", "Display the program version.", {"version"});

  sentinel::CliConfigParser cliParser(parser);

  try {
    parser.helpParams.showTerminator = false;
    parser.helpParams.addDefault = false;
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
  } catch (const sentinel::ThresholdError&) {
    return 3;
  } catch (const std::exception& e) {
    sentinel::Logger::error("{}", e.what());
    return 1;
  }
}
