/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include "sentinel/Console.hpp"
#include "sentinel/stages/InitStage.hpp"

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

bool InitStage::execute() {
  if (!mConfig.init) {
    return true;
  }

  static const char* const kConfigFileName = "sentinel.yaml";
  if (fs::exists(kConfigFileName)) {
    bool overwrite = mConfig.force && *mConfig.force;
    if (!overwrite) {
      overwrite = Console::confirm("'{}' already exists. Overwrite?", kConfigFileName);
    }
    if (!overwrite) {
      Console::out("Aborted.");
      return false;
    }
  }
  std::ofstream out(kConfigFileName);
  if (!out) {
    throw std::runtime_error(fmt::format("Failed to create '{}'", kConfigFileName));
  }
  out << kYamlTemplate;
  Console::out("Generated '{}'", kConfigFileName);
  return false;
}

}  // namespace sentinel
