/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>  // NOLINT
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/YamlConfigParser.hpp"

namespace fs = std::filesystem;

namespace sentinel {

static std::vector<std::string> toStringVector(const YAML::Node& node, const std::string& key) {
  if (!node.IsSequence()) {
    throw std::runtime_error(fmt::format("Config key '{}' must be a list", key));
  }
  std::vector<std::string> result(node.size());
  std::transform(node.begin(), node.end(), result.begin(),
                 [](const YAML::Node& item) { return item.as<std::string>(); });
  return result;
}

Config YamlConfigParser::loadFromFile(const std::string& path) {
  YAML::Node root;
  try {
    root = YAML::LoadFile(path);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Failed to parse config file '{}': {}", path, e.what()));
  }

  if (!root.IsMap()) {
    throw std::runtime_error(fmt::format("Config file '{}': expected a YAML mapping at root", path));
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
      cfg.extensions = toStringVector(root["extension"], "extension");
    }
    if (root["pattern"]) {
      cfg.patterns = toStringVector(root["pattern"], "pattern");
    }
    if (root["exclude"]) {
      cfg.excludes = toStringVector(root["exclude"], "exclude");
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
    if (root["test-report-dir"]) {
      cfg.testResultDir = root["test-report-dir"].as<std::string>();
    }
    if (root["test-report-extension"]) {
      cfg.testResultFileExts = toStringVector(root["test-report-extension"], "test-report-extension");
    }
    if (root["coverage"]) {
      cfg.coverageFiles = toStringVector(root["coverage"], "coverage");
    }
    if (root["generator"]) {
      cfg.generator = root["generator"].as<std::string>();
    }
    if (root["timeout"]) {
      cfg.timeLimit = root["timeout"].as<std::string>();
    }
    if (root["kill-after"]) {
      cfg.killAfter = root["kill-after"].as<std::string>();
    }
    if (root["seed"]) {
      cfg.seed = root["seed"].as<unsigned>();
    }
    if (root["operator"]) {
      cfg.operators = toStringVector(root["operator"], "operator");
    }
    if (root["threshold"]) {
      cfg.threshold = root["threshold"].as<double>();
    }
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Config file '{}': {}", path, e.what()));
  }

  // Path resolution logic is moved to ConfigResolver in the later step,
  // but for now, we keep the basic parsing here.

  return cfg;
}

}  // namespace sentinel
