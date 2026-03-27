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
  // Precondition: buildCmd, testCmd, testResultDir are always set by ConfigResolver.
  if (config.buildCmd->empty()) {
    throw InvalidArgumentException("Option --build-command is required.");
  }
  if (config.testCmd->empty()) {
    throw InvalidArgumentException("Option --test-command is required.");
  }
  if (config.testResultDir->empty()) {
    throw InvalidArgumentException("Option --test-result-dir is required.");
  }

  if (config.threshold && (*config.threshold < 0.0 || *config.threshold > 100.0)) {
    throw InvalidArgumentException(
        fmt::format("Invalid --threshold value: {:.1f}. Expected a percentage in [0, 100].", *config.threshold));
  }

  if (config.partition && !config.partition->empty()) {
    try {
      Partition::parse(*config.partition);
    } catch (const std::invalid_argument& e) {
      throw InvalidArgumentException(e.what());
    }
    if (!config.seed) {
      throw InvalidArgumentException("--partition requires an explicit --seed value.");
    }
  }

  checkWarnings(config);
}

void ConfigValidator::checkWarnings(const Config& config) {
  std::vector<std::string> warnings;

  if (config.limit && *config.limit == 0) {
    warnings.push_back("limit: 0 - all candidate mutants will be evaluated. This may take a very long time.");
  }

  if (config.timeout && *config.timeout == "0") {
    warnings.push_back("timeout: 0 - no per-mutant test time limit. A hanging test will block the run indefinitely.");
  }

  if (config.excludes) {
    for (const auto& excl : *config.excludes) {
      if (!excl.empty() && excl.back() == '/') {
        warnings.push_back(
            fmt::format("exclude: '{}' ends with '/'. "
                        "Patterns are matched against file paths, not directories.",
                        excl));
      } else if (!excl.empty() && excl.front() != '*' && !fs::path(excl).is_absolute()) {
        warnings.push_back(fmt::format("exclude: '{}' is a relative pattern without a leading '*'.", excl));
      }
    }
  }

  if (config.patterns) {
    fs::path srcRoot = config.sourceDir ? *config.sourceDir : fs::current_path();
    for (const auto& pat : *config.patterns) {
      fs::path patPath(pat);
      if (patPath.is_absolute()) {
        auto rel = patPath.lexically_relative(srcRoot);
        if (rel.empty() || rel.native().find("..") != std::string::npos) {
          warnings.push_back(fmt::format("pattern: '{}' is an absolute path outside source-dir.", pat));
        } else {
          warnings.push_back(
              fmt::format("pattern: '{}' is an absolute path. "
                          "Git pathspec uses paths relative to repository root.",
                          pat));
        }
      }
    }
  }

  if (!warnings.empty()) {
    Logger::warn("Configuration warnings:");
    for (const auto& w : warnings) {
      Logger::warn("  {}", w);
    }
  }
}

}  // namespace sentinel
