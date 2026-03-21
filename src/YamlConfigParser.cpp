/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/YamlConfigParser.hpp"

namespace sentinel {

namespace fs = std::filesystem;

template <typename T>
static std::vector<T> toVector(const YAML::Node& node, const std::string& key) {
  if (!node.IsSequence()) {
    throw std::runtime_error(fmt::format("Config key '{}' must be a list", key));
  }
  std::vector<T> result;
  result.reserve(node.size());
  for (const auto& item : node) result.push_back(T(item.as<std::string>()));
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
    throw std::runtime_error(fmt::format(
        "Config file '{}': missing required field 'version'. "
        "Add 'version: {}' to the top of your config file.",
        path.string(), kSupportedVersion));
  }
  int version = 0;
  try {
    version = root["version"].as<int>();
  } catch (const YAML::Exception&) {
    throw std::runtime_error(fmt::format(
        "Config file '{}': 'version' must be an integer. "
        "Add 'version: {}' to the top of your config file.",
        path.string(), kSupportedVersion));
  }
  if (version != kSupportedVersion) {
    throw std::runtime_error(fmt::format(
        "Config file '{}': unsupported version {}. This sentinel supports version {}.",
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
