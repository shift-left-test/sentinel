/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/YamlConfigParser.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static const char* const kYamlTemplate =
    "# sentinel.yaml - full configuration template\n"
    "#\n"
    "# Uncomment and edit the options you need.\n"
    "# CLI arguments always take priority over values in this file.\n"
    "\n"
    "# Config file format version (required)\n"
    "version: 1\n"
    "\n"
    "# Directory for output reports (default: none)\n"
    "# output-dir: ./sentinel_output\n"
    "\n"
    "# Workspace directory for all sentinel run artifacts (default: ./.sentinel)\n"
    "# workspace: ./.sentinel\n"
    "\n"
    "# --- Run options ---\n"
    "\n"
    "# Fail with exit code 3 if mutation score is below this percentage 0-100 (default: disabled)\n"
    "# threshold: 80\n"
    "\n"
    "# Evaluate only the N-th slice of all mutants out of TOTAL partitions (e.g. 2/4); requires --seed\n"
    "# partition: 1/1\n"
    "\n"
    "# --- Build & test options ---\n"
    "\n"
    "# Source root directory (default: .)\n"
    "# source-dir: .\n"
    "\n"
    "# Shell command to build the source\n"
    "# build-command: make\n"
    "\n"
    "# Path to directory containing compile_commands.json (default: .)\n"
    "# compiledb-dir: .\n"
    "\n"
    "# Shell command to execute tests\n"
    "# test-command: make test\n"
    "\n"
    "# Path to the test report directory\n"
    "# test-result-dir: ./test-results\n"
    "\n"
    "# File extension of the test report (default: xml)\n"
    "# test-result-ext:\n"
    "#   - xml\n"
    "\n"
    "# Test time limit in seconds (default: auto - 2x baseline run time; 0 = no limit)\n"
    "# timeout: auto\n"
    "\n"
    "# Seconds to wait after timeout before sending SIGKILL (default: 60; 0 = disabled)\n"
    "# kill-after: 60\n"
    "\n"
    "# --- Mutation options ---\n"
    "\n"
    "# Diff scope: 'commit' (changed lines only) or 'all' (entire codebase) (default: all)\n"
    "# scope: all\n"
    "\n"
    "# Source file extensions to mutate (default: cxx cpp cc c c++ cu)\n"
    "# extension:\n"
    "#   - cpp\n"
    "#   - cxx\n"
    "#   - cc\n"
    "#   - c\n"
    "#   - c++\n"
    "#   - cu\n"
    "\n"
    "# Paths or glob patterns to constrain the diff (default: none - entire source)\n"
    "# pattern: []\n"
    "\n"
    "# Paths excluded from mutation; fnmatch-style patterns (default: none)\n"
    "# exclude: []\n"
    "\n"
    "# Maximum number of mutants to generate (default: 0 = unlimited)\n"
    "# limit: 0\n"
    "\n"
    "# Mutant selection strategy (default: uniform)\n"
    "#   uniform  - one mutant per operator per source line\n"
    "#   random   - randomly sampled from all possible mutants\n"
    "#   weighted - samples more mutants from complex code\n"
    "# generator: uniform\n"
    "\n"
    "# Random seed for mutant selection (default: auto - picked randomly)\n"
    "# seed: auto\n"
    "\n"
    "# Mutation operators to use; omit to use all operators (default: all)\n"
    "#   AOR - Arithmetic Operator Replacement  (+, -, *, /)\n"
    "#   BOR - Bitwise Operator Replacement      (&, |, ^)\n"
    "#   LCR - Logical Connector Replacement     (&&, ||)\n"
    "#   ROR - Relational Operator Replacement   (<, >, ==, !=)\n"
    "#   SDL - Statement Deletion\n"
    "#   SOR - Shift Operator Replacement        (<<, >>)\n"
    "#   UOI - Unary Operator Insertion          (-x, !x)\n"
    "# operator:\n"
    "#   - AOR\n"
    "#   - BOR\n"
    "#   - LCR\n"
    "#   - ROR\n"
    "#   - SDL\n"
    "#   - SOR\n"
    "#   - UOI\n"
    "\n"
    "# lcov-format coverage result files; limits mutation to covered lines only (default: none)\n"
    "# coverage: []\n";

void YamlConfigParser::writeTemplate(const std::filesystem::path& path) {
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error(fmt::format("Failed to create '{}'", path.string()));
  }
  out << kYamlTemplate;
  Console::out("Generated '{}'", path.string());
}

template <typename T>
static std::vector<T> toVector(const YAML::Node& node, const std::string& key) {
  if (!node.IsSequence()) {
    throw std::runtime_error(fmt::format("Config key '{}' must be a list", key));
  }
  std::vector<T> result;
  result.reserve(node.size());
  std::transform(node.begin(), node.end(), std::back_inserter(result),
                 [](const YAML::Node& item) { return T(item.as<std::string>()); });
  return result;
}

Config YamlConfigParser::loadFromFile(const std::filesystem::path& path) {
  YAML::Node root;
  try {
    root = YAML::LoadFile(path.string());
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Failed to parse config file '{}': {}", path.string(), e.what()));
  }

  if (!root.IsMap()) {
    throw std::runtime_error(fmt::format("Config file '{}': expected a YAML mapping at root", path.string()));
  }

  static constexpr int kSupportedVersion = 1;
  if (!root["version"]) {
    throw std::runtime_error(
        fmt::format("Config file '{}': missing required field 'version'. "
                    "Add 'version: {}' to the top of your config file.",
                    path.string(), kSupportedVersion));
  }
  int version = 0;
  try {
    version = root["version"].as<int>();
  } catch (const YAML::Exception&) {
    throw std::runtime_error(
        fmt::format("Config file '{}': 'version' must be an integer. "
                    "Add 'version: {}' to the top of your config file.",
                    path.string(), kSupportedVersion));
  }
  if (version != kSupportedVersion) {
    throw std::runtime_error(fmt::format("Config file '{}': unsupported version {}. This sentinel supports version {}.",
                                         path.string(), version, kSupportedVersion));
  }

  Config cfg;

  try {
    // Shared with Command base class
    if (root["source-dir"]) {
      cfg.sourceDir = root["source-dir"].as<std::string>();
    }
    if (root["workspace"]) {
      cfg.workDir = root["workspace"].as<std::string>();
    }
    if (root["output-dir"]) {
      cfg.outputDir = root["output-dir"].as<std::string>();
    }

    // CommandRun-specific options
    if (root["compiledb-dir"]) {
      cfg.compileDbDir = root["compiledb-dir"].as<std::string>();
    }
    if (root["scope"]) {
      cfg.scope = root["scope"].as<std::string>();
    }
    if (root["extension"]) {
      cfg.extensions = toVector<std::string>(root["extension"], "extension");
    }
    if (root["pattern"]) {
      cfg.patterns = toVector<std::string>(root["pattern"], "pattern");
    }
    if (root["exclude"]) {
      cfg.excludes = toVector<std::string>(root["exclude"], "exclude");
    }
    if (root["limit"]) {
      cfg.limit = root["limit"].as<size_t>();
    }
    if (root["build-command"]) {
      cfg.buildCmd = root["build-command"].as<std::string>();
    }
    if (root["test-command"]) {
      cfg.testCmd = root["test-command"].as<std::string>();
    }
    if (root["test-result-dir"]) {
      cfg.testResultDir = root["test-result-dir"].as<std::string>();
    }
    if (root["test-result-ext"]) {
      cfg.testResultExts = toVector<std::string>(root["test-result-ext"], "test-result-ext");
    }
    if (root["coverage"]) {
      cfg.coverageFiles = toVector<fs::path>(root["coverage"], "coverage");
    }
    if (root["generator"]) {
      cfg.generator = root["generator"].as<std::string>();
    }
    if (root["timeout"]) {
      cfg.timeout = root["timeout"].as<std::string>();
    }
    if (root["kill-after"]) {
      cfg.killAfter = root["kill-after"].as<std::string>();
    }
    if (root["seed"]) {
      cfg.seed = root["seed"].as<unsigned>();
    }
    if (root["operator"]) {
      cfg.operators = toVector<std::string>(root["operator"], "operator");
    }
    if (root["threshold"]) {
      cfg.threshold = root["threshold"].as<double>();
    }
    if (root["partition"]) {
      cfg.partition = root["partition"].as<std::string>();
    }
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Config file '{}': {}", path.string(), e.what()));
  }

  return cfg;
}

}  // namespace sentinel
