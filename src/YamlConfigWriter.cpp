/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <cerrno>
#include <cstring>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include "sentinel/Console.hpp"
#include "sentinel/YamlConfigWriter.hpp"

namespace sentinel {

static const char* const kYamlTemplate =
    "# sentinel.yaml - full configuration template\n"
    "#\n"
    "# Uncomment and edit the options you need.\n"
    "# CLI arguments always take priority over values in this file.\n"
    "\n"
    "## Config file format version (required)\n"
    "version: 1\n"
    "\n"
    "## Directory for output reports (default: none)\n"
    "# output-dir: ./sentinel_output\n"
    "\n"
    "# --- Build & test options ---\n"
    "\n"
    "## Source root directory (default: .)\n"
    "# source-dir: .\n"
    "\n"
    "## Shell command to build the source\n"
    "# build-command: make\n"
    "\n"
    "## Path to directory containing compile_commands.json (default: .)\n"
    "# compiledb-dir: .\n"
    "\n"
    "## Shell command to execute tests\n"
    "# test-command: make test\n"
    "\n"
    "## Path to the test report directory\n"
    "# test-result-dir: ./test-results\n"
    "\n"
    "## Test time limit in seconds (default: 1.5x original test time; 0 = no limit)\n"
    "# timeout: 60\n"
    "\n"
    "# --- Mutation options ---\n"
    "\n"
    "## Source file extensions to mutate (default: cxx cpp cc c c++ cu)\n"
    "# extension:\n"
    "#   - cpp\n"
    "#   - cxx\n"
    "#   - cc\n"
    "#   - c\n"
    "#   - c++\n"
    "#   - cu\n"
    "\n"
    "## Paths or glob patterns to constrain mutation scope (default: none - entire source)\n"
    "## Prefix a pattern with ! to exclude matching files (e.g. \"!*/test/*\")\n"
    "# pattern: []\n"
    "\n"
    "## Mutant selection strategy (default: uniform)\n"
    "##   uniform  - one mutant per operator per source line\n"
    "##   random   - randomly sampled from all possible mutants\n"
    "##   weighted - samples more mutants from complex code\n"
    "# generator: uniform\n"
    "\n"
    "## Maximum number of mutants per source line (default: 1; 0 = unlimited)\n"
    "# mutants-per-line: 1\n"
    "\n"
    "## Mutation operators to use; omit to use all operators (default: all)\n"
    "# operator:\n"
    "#   - AOR  # Arithmetic Operator Replacement  (+, -, *, /)\n"
    "#   - BOR  # Bitwise Operator Replacement     (&, |, ^)\n"
    "#   - LCR  # Logical Connector Replacement    (&&, ||)\n"
    "#   - ROR  # Relational Operator Replacement  (<, >, ==, !=)\n"
    "#   - SDL  # Statement Deletion\n"
    "#   - SOR  # Shift Operator Replacement       (<<, >>)\n"
    "#   - UOI  # Unary Operator Insertion         (-x, !x)\n"
    "\n"
    "# --- Advanced options ---\n"
    "\n"
    "## lcov tracefiles; skip evaluation for uncovered mutants (default: none)\n"
    "# lcov-tracefile: []\n";

void YamlConfigWriter::writeTemplate(const std::filesystem::path& path) {
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error(fmt::format("Failed to create '{}': {}", path, std::strerror(errno)));
  }
  out << kYamlTemplate;
  Console::out("Created '{}'", path);
}

}  // namespace sentinel
