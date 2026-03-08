/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <experimental/filesystem>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/SentinelConfig.hpp"

namespace sentinel {

namespace {

namespace fs = std::experimental::filesystem;

// Equivalent of path::lexically_normal() for experimental::filesystem,
// which lacks that method. Resolves "." and ".." components without
// requiring the path to exist on the filesystem.
static fs::path lexicallyNormal(const fs::path& p) {
  fs::path result;
  for (const auto& part : p) {
    if (part == ".") {
      continue;
    } else if (part == "..") {
      if (!result.empty() && result.filename() != "..") {
        result = result.parent_path();
      } else {
        result /= part;
      }
    } else {
      result /= part;
    }
  }
  return result.empty() ? fs::path(".") : result;
}

std::vector<std::string> toStringVector(const YAML::Node& node, const std::string& key) {
  if (!node.IsSequence()) {
    throw std::runtime_error(fmt::format("Config key '{}' must be a list", key));
  }
  std::vector<std::string> result(node.size());
  std::transform(node.begin(), node.end(), result.begin(),
                 [](const YAML::Node& item) { return item.as<std::string>(); });
  return result;
}

}  // namespace

SentinelConfig SentinelConfig::loadFromFile(const std::string& path) {
  YAML::Node root;
  try {
    root = YAML::LoadFile(path);
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Failed to parse config file '{}': {}", path, e.what()));
  }

  if (!root.IsMap()) {
    throw std::runtime_error(fmt::format("Config file '{}': expected a YAML mapping at root", path));
  }

  SentinelConfig cfg;

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

  // Resolve relative path fields relative to the YAML file's directory,
  // consistent with tools like Docker Compose and Ansible.
  fs::path yamlDir = fs::absolute(fs::path(path)).parent_path();

  auto resolvePath = [&](std::optional<std::string>& optPath) {
    if (optPath && !optPath->empty()) {
      fs::path p(*optPath);
      if (p.is_relative()) {
        *optPath = lexicallyNormal(yamlDir / p).string();
      }
    }
  };

  resolvePath(cfg.sourceDir);
  resolvePath(cfg.workDir);
  resolvePath(cfg.outputDir);
  resolvePath(cfg.compileDbDir);
  resolvePath(cfg.testResultDir);

  if (cfg.coverageFiles) {
    for (auto& f : *cfg.coverageFiles) {
      fs::path p(f);
      if (p.is_relative()) {
        f = lexicallyNormal(yamlDir / p).string();
      }
    }
  }

  return cfg;
}

}  // namespace sentinel
