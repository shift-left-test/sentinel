/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <yaml-cpp/yaml.h>
#include <filesystem>  // NOLINT
#include <stdexcept>
#include <string>
#include "sentinel/Config.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Generator parseGenerator(const std::string& s) {
  std::string lower = string::toLower(s);
  if (lower == "uniform") return Generator::UNIFORM;
  if (lower == "random") return Generator::RANDOM;
  if (lower == "weighted") return Generator::WEIGHTED;
  throw std::invalid_argument(
      fmt::format("Invalid generator: '{}'. Expected 'uniform', 'random', or 'weighted'.", s));
}

std::string generatorToString(Generator gen) {
  switch (gen) {
    case Generator::UNIFORM: return "uniform";
    case Generator::RANDOM: return "random";
    case Generator::WEIGHTED: return "weighted";
  }
  throw std::invalid_argument("Unknown Generator value");
}

Config Config::withDefaults() {
  Config cfg;
  cfg.sourceDir = fs::absolute(".").lexically_normal();
  cfg.workDir = fs::absolute(".sentinel").lexically_normal();
  cfg.compileDbDir = fs::absolute(".").lexically_normal();
  return cfg;
}

std::ostream& operator<<(std::ostream& out, const Config& cfg) {
  YAML::Emitter emitter;
  emitter << YAML::BeginMap;
  emitter << YAML::Key << "version" << YAML::Value << 1;
  emitter << YAML::Key << "source-dir" << YAML::Value << cfg.sourceDir.string();
  if (!cfg.outputDir.empty()) {
    emitter << YAML::Key << "output-dir" << YAML::Value << cfg.outputDir.string();
  }
  emitter << YAML::Key << "compiledb-dir" << YAML::Value << cfg.compileDbDir.string();
  if (cfg.from) {
    emitter << YAML::Key << "from" << YAML::Value << *cfg.from;
  }
  emitter << YAML::Key << "extension" << YAML::Value << YAML::BeginSeq;
  for (const auto& e : cfg.extensions) emitter << e;
  emitter << YAML::EndSeq;
  emitter << YAML::Key << "pattern" << YAML::Value << YAML::BeginSeq;
  for (const auto& p : cfg.patterns) emitter << p;
  emitter << YAML::EndSeq;
  emitter << YAML::Key << "build-command" << YAML::Value << cfg.buildCmd;
  emitter << YAML::Key << "test-command" << YAML::Value << cfg.testCmd;
  emitter << YAML::Key << "test-result-dir" << YAML::Value << cfg.testResultDir.string();
  emitter << YAML::Key << "lcov-tracefile" << YAML::Value << YAML::BeginSeq;
  for (const auto& c : cfg.lcovTracefiles) emitter << c.string();
  emitter << YAML::EndSeq;
  emitter << YAML::Key << "generator" << YAML::Value << generatorToString(cfg.generator);
  if (cfg.mutantsPerLine != 1) {
    emitter << YAML::Key << "mutants-per-line" << YAML::Value << cfg.mutantsPerLine;
  }
  if (cfg.timeout) {
    emitter << YAML::Key << "timeout" << YAML::Value << *cfg.timeout;
  }
  emitter << YAML::Key << "operator" << YAML::Value << YAML::BeginSeq;
  for (const auto& op : cfg.operators) emitter << op;
  emitter << YAML::EndSeq;
  emitter << YAML::EndMap;
  out << emitter.c_str();
  return out;
}

}  // namespace sentinel
