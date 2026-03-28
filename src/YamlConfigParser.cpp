/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/YamlConfigParser.hpp"
#include "sentinel/util/formatter.hpp"

namespace sentinel {

namespace fs = std::filesystem;

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

static fs::path resolvePath(const fs::path& base, const std::string& value) {
  fs::path p(value);
  if (p.is_relative()) {
    return (base / p).lexically_normal();
  }
  return p.lexically_normal();
}

void YamlConfigParser::applyTo(Config* cfg, const std::filesystem::path& path) {
  YAML::Node root;
  try {
    root = YAML::LoadFile(path.string());
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Failed to parse config file '{}': {}", path, e.what()));
  }

  if (!root.IsMap()) {
    throw std::runtime_error(fmt::format("Config file '{}': expected a YAML mapping at root", path));
  }

  static constexpr int kSupportedVersion = 1;
  if (!root["version"]) {
    throw std::runtime_error(
        fmt::format("Config file '{}': missing required field 'version'. "
                    "Add 'version: {}' to the top of your config file.",
                    path, kSupportedVersion));
  }
  int version = 0;
  try {
    version = root["version"].as<int>();
  } catch (const YAML::Exception&) {
    throw std::runtime_error(
        fmt::format("Config file '{}': 'version' must be an integer. "
                    "Add 'version: {}' to the top of your config file.",
                    path, kSupportedVersion));
  }
  if (version > kSupportedVersion) {
    throw std::runtime_error(fmt::format(
        "Config file '{}': version {} is newer than supported version {}. "
        "Please upgrade sentinel.",
        path, version, kSupportedVersion));
  }
  if (version < kSupportedVersion) {
    throw std::runtime_error(fmt::format(
        "Config file '{}': version {} is outdated. "
        "Run `sentinel --init` to regenerate the config file.",
        path, version, kSupportedVersion));
  }

  const fs::path base = fs::absolute(path).parent_path();

  try {
    if (root["source-dir"]) cfg->sourceDir = resolvePath(base, root["source-dir"].as<std::string>());
    if (root["workspace"]) cfg->workDir = resolvePath(base, root["workspace"].as<std::string>());
    if (root["output-dir"]) cfg->outputDir = resolvePath(base, root["output-dir"].as<std::string>());
    if (root["compiledb-dir"]) cfg->compileDbDir = resolvePath(base, root["compiledb-dir"].as<std::string>());
    if (root["test-result-dir"]) cfg->testResultDir = resolvePath(base, root["test-result-dir"].as<std::string>());

    if (root["build-command"]) cfg->buildCmd = root["build-command"].as<std::string>();
    if (root["test-command"]) cfg->testCmd = root["test-command"].as<std::string>();
    if (root["test-result-ext"]) {
      cfg->testResultExts = toVector<std::string>(root["test-result-ext"], "test-result-ext");
    }
    if (root["timeout"]) cfg->timeout = root["timeout"].as<size_t>();
    if (root["kill-after"]) cfg->killAfter = root["kill-after"].as<size_t>();

    if (root["scope"]) cfg->scope = root["scope"].as<std::string>();
    if (root["extension"]) cfg->extensions = toVector<std::string>(root["extension"], "extension");
    if (root["pattern"]) cfg->patterns = toVector<std::string>(root["pattern"], "pattern");
    if (root["generator"]) cfg->generator = root["generator"].as<std::string>();
    if (root["operator"]) cfg->operators = toVector<std::string>(root["operator"], "operator");
    if (root["coverage"]) {
      auto files = toVector<fs::path>(root["coverage"], "coverage");
      cfg->coverageFiles.clear();
      for (const auto& f : files) {
        cfg->coverageFiles.push_back(resolvePath(base, f.string()));
      }
    }
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Config file '{}': {}", path, e.what()));
  }
}

}  // namespace sentinel
