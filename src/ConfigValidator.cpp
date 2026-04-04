/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/ConfigValidator.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

void ConfigValidator::validate(const Config& config) {
  if (!config.mergeWorkspaces.empty()) {
    return;
  }

  // Precondition: buildCmd, testCmd, testResultDir are always set by ConfigResolver.
  if (config.buildCmd.empty()) {
    throw InvalidArgumentException("Option --build-command is required.");
  }
  if (config.testCmd.empty()) {
    throw InvalidArgumentException("Option --test-command is required.");
  }
  if (config.testResultDir.empty()) {
    throw InvalidArgumentException("Option --test-result-dir is required.");
  }

  if (config.threshold && (*config.threshold < 0.0 || *config.threshold > 100.0)) {
    throw InvalidArgumentException(
        fmt::format("Invalid --threshold value: {:.1f}. Expected a percentage in [0, 100].", *config.threshold));
  }

  if (config.partition) {
    try {
      Partition::parse(*config.partition);
    } catch (const std::invalid_argument& e) {
      throw InvalidArgumentException(e.what());
    }
  }

  checkWarnings(config);
}

void ConfigValidator::checkWarnings(const Config& config) {
  std::vector<std::string> warnings;

  if (config.partition && !config.seed) {
    warnings.push_back("--partition: --seed is not set. A random seed will be used, "
                        "so each run may evaluate a different subset of mutants.");
  }

  if (config.timeout && *config.timeout == 0) {
    warnings.push_back("--timeout: 0 - no per-mutant test time limit. A hanging test will block the run indefinitely.");
  }

  fs::path srcRoot = config.sourceDir;
  for (const auto& pat : config.patterns) {
    if (pat.empty()) {
      continue;
    }

    const bool isNegation = pat.front() == '!';
    const std::string checkPat = isNegation ? pat.substr(1) : pat;

    if (isNegation && !checkPat.empty() && checkPat.back() == '/') {
      warnings.push_back(
          fmt::format("--pattern: '{}' ends with '/'. "
                      "Patterns are matched against file paths, not directories.",
                      pat));
    }

    fs::path patPath(checkPat);
    if (patPath.is_absolute()) {
      auto rel = patPath.lexically_relative(srcRoot);
      if (rel.empty() || rel.native().find("..") != std::string::npos) {
        warnings.push_back(fmt::format("--pattern: '{}' is an absolute path outside source-dir.", pat));
      } else {
        warnings.push_back(
            fmt::format("--pattern: '{}' is an absolute path. "
                        "Patterns use paths relative to repository root.",
                        pat));
      }
    }
  }

  for (const auto& w : warnings) {
    Logger::warn("{}", w);
  }
}

}  // namespace sentinel
