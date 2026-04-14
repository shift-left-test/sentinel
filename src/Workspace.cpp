/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Workspace.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Workspace::Workspace(const fs::path& root) : mRoot(root) {
}

void Workspace::restoreBackup(const fs::path& srcRoot) {
  const fs::path backup = getBackupDir();
  if (!fs::is_directory(backup)) return;
  for (const auto& dirent : fs::directory_iterator(backup)) {
    fs::copy(dirent.path(), srcRoot / dirent.path().filename(),
             fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    fs::remove_all(dirent.path());
  }
}

bool Workspace::hasPreviousRun() const {
  return fs::exists(mRoot / "config.yaml");
}

void Workspace::initialize() {
  fs::remove_all(mRoot);
  fs::create_directories(getOriginalResultsDir());
  fs::create_directories(getBackupDir());
}

void Workspace::saveConfig(const Config& cfg) {
  std::ofstream out(mRoot / "config.yaml");
  if (!out) {
    throw std::runtime_error(fmt::format(
        "Failed to write workspace config '{}': {}",
        (mRoot / "config.yaml").string(), std::strerror(errno)));
  }
  out << cfg;
}

std::ostream& operator<<(std::ostream& out, const WorkspaceStatus& status) {
  YAML::Emitter emitter;
  emitter << YAML::BeginMap;
  if (status.version.has_value()) {
    emitter << YAML::Key << "version" << YAML::Value << *status.version;
  }
  if (status.originalTime.has_value()) {
    emitter << YAML::Key << "original-time" << YAML::Value << *status.originalTime;
  }
  if (status.candidateCount.has_value()) {
    emitter << YAML::Key << "candidate-count" << YAML::Value << *status.candidateCount;
  }
  if (status.partIndex.has_value()) {
    emitter << YAML::Key << "part-index" << YAML::Value << *status.partIndex;
  }
  if (status.partCount.has_value()) {
    emitter << YAML::Key << "part-count" << YAML::Value << *status.partCount;
  }
  if (status.seed.has_value()) {
    emitter << YAML::Key << "seed" << YAML::Value << *status.seed;
  }
  if (status.from.has_value()) {
    emitter << YAML::Key << "from" << YAML::Value << *status.from;
  }
  if (status.uncommitted.has_value()) {
    emitter << YAML::Key << "uncommitted" << YAML::Value << *status.uncommitted;
  }
  if (status.limit.has_value()) {
    emitter << YAML::Key << "limit" << YAML::Value << *status.limit;
  }
  if (status.mergedPartitions.has_value()) {
    emitter << YAML::Key << "merged-partitions" << YAML::Value << YAML::BeginSeq;
    for (auto idx : *status.mergedPartitions) {
      emitter << idx;
    }
    emitter << YAML::EndSeq;
  }
  emitter << YAML::EndMap;
  out << emitter.c_str();
  return out;
}

std::istream& operator>>(std::istream& in, WorkspaceStatus& status) {
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (content.empty()) {
    in.setstate(std::ios::failbit);
    return in;
  }
  try {
    YAML::Node node = YAML::Load(content);
    if (node["version"]) status.version = node["version"].as<std::string>();
    if (node["original-time"]) status.originalTime = node["original-time"].as<std::size_t>();
    if (node["candidate-count"]) status.candidateCount = node["candidate-count"].as<std::size_t>();
    if (node["part-index"]) status.partIndex = node["part-index"].as<std::size_t>();
    if (node["part-count"]) status.partCount = node["part-count"].as<std::size_t>();
    if (node["seed"]) status.seed = node["seed"].as<unsigned int>();
    if (node["from"]) status.from = node["from"].as<std::string>();
    if (node["uncommitted"]) status.uncommitted = node["uncommitted"].as<bool>();
    if (node["limit"]) status.limit = node["limit"].as<std::size_t>();
    if (node["merged-partitions"]) {
      const auto& seq = node["merged-partitions"];
      std::vector<std::size_t> parts(seq.size());
      std::transform(seq.begin(), seq.end(), parts.begin(),
                     [](const YAML::Node& n) { return n.as<std::size_t>(); });
      status.mergedPartitions = parts;
    }
  } catch (const YAML::Exception&) {
    in.setstate(std::ios::failbit);
  }
  return in;
}

void Workspace::saveStatus(const WorkspaceStatus& status) {
  WorkspaceStatus current;
  fs::path p = mRoot / "status.yaml";
  if (fs::exists(p)) {
    current = loadStatus();
  }
  if (status.version.has_value()) current.version = status.version;
  if (status.originalTime.has_value()) current.originalTime = status.originalTime;
  if (status.candidateCount.has_value()) current.candidateCount = status.candidateCount;
  if (status.partIndex.has_value()) current.partIndex = status.partIndex;
  if (status.partCount.has_value()) current.partCount = status.partCount;
  if (status.mergedPartitions.has_value()) current.mergedPartitions = status.mergedPartitions;
  if (status.seed.has_value()) current.seed = status.seed;
  if (status.from.has_value()) current.from = status.from;
  if (status.uncommitted.has_value()) current.uncommitted = status.uncommitted;
  if (status.limit.has_value()) current.limit = status.limit;

  std::ofstream f(p);
  if (!f) {
    throw std::runtime_error(fmt::format("Failed to write status.yaml '{}': {}", p.string(), std::strerror(errno)));
  }
  f << current;
}

WorkspaceStatus Workspace::loadStatus() const {
  WorkspaceStatus status;
  fs::path p = mRoot / "status.yaml";
  if (!fs::exists(p)) return status;
  std::ifstream f(p);
  if (!(f >> status)) {
    throw std::runtime_error(fmt::format("Failed to read status.yaml '{}': {}", p.string(), std::strerror(errno)));
  }
  return status;
}

const fs::path& Workspace::getRoot() const {
  return mRoot;
}

fs::path Workspace::getOriginalDir() const {
  return mRoot / "original";
}

fs::path Workspace::getOriginalResultsDir() const {
  return mRoot / "original" / "results";
}

fs::path Workspace::getBackupDir() const {
  return mRoot / "backup";
}

fs::path Workspace::getActualDir() const {
  return mRoot / "actual";
}

fs::path Workspace::getOriginalBuildLog() const {
  return mRoot / "original" / "build.log";
}

fs::path Workspace::getOriginalTestLog() const {
  return mRoot / "original" / "test.log";
}

fs::path Workspace::getMutantDir(int id) const {
  return mRoot / mutantDirName(id);
}

fs::path Workspace::getMutantBuildLog(int id) const {
  return mutantFile(id, "build.log");
}

fs::path Workspace::getMutantTestLog(int id) const {
  return mutantFile(id, "test.log");
}

void Workspace::createMutant(int id, const Mutant& m) {
  fs::path dir = getMutantDir(id);
  fs::create_directories(dir);
  std::ofstream out(dir / "mt.cfg");
  if (!out) {
    throw std::runtime_error(fmt::format("Failed to write mt.cfg for mutant {}: {}", id, std::strerror(errno)));
  }
  out << m;
}

bool Workspace::isLocked(int id) const {
  return fs::exists(mutantFile(id, "mt.lock"));
}

void Workspace::setLock(int id) {
  std::ofstream f(mutantFile(id, "mt.lock"));
  if (!f) {
    throw std::runtime_error(fmt::format("Failed to create lock for mutant {}: {}", id, std::strerror(errno)));
  }
}

void Workspace::clearLock(int id) {
  fs::remove(mutantFile(id, "mt.lock"));
}

bool Workspace::isDone(int id) const {
  return fs::exists(mutantFile(id, "mt.done"));
}

bool Workspace::isComplete() const {
  return fs::exists(getCompleteMarker());
}

void Workspace::setComplete() {
  std::ofstream f(getCompleteMarker());
  if (!f) {
    throw std::runtime_error(fmt::format(
        "Failed to create run.done in '{}': {}",
        mRoot.string(), std::strerror(errno)));
  }
}

void Workspace::setDone(int id, const MutationResult& result) {
  std::ofstream out(mutantFile(id, "mt.done"));
  if (!out) {
    throw std::runtime_error(fmt::format("Failed to write mt.done for mutant {}: {}", id, std::strerror(errno)));
  }
  out << result;
}

MutationResult Workspace::getDoneResult(int id) const {
  std::ifstream in(mutantFile(id, "mt.done"));
  if (!in) {
    throw std::runtime_error(fmt::format("Failed to read mt.done for mutant {}: {}", id, std::strerror(errno)));
  }
  MutationResult result;
  if (!(in >> result)) {
    throw std::runtime_error(fmt::format("Failed to parse mt.done for mutant {}", id));
  }
  return result;
}

bool Workspace::hasMutants() const {
  if (!fs::is_directory(mRoot)) {
    return false;
  }
  for (const auto& entry : fs::directory_iterator(mRoot)) {
    if (!fs::is_directory(entry.path())) {
      continue;
    }
    const std::string name = entry.path().filename().string();
    auto predicate = [](unsigned char c) { return std::isdigit(c) != 0; };
    if (name.empty() || !std::all_of(name.begin(), name.end(), predicate)) {
      continue;
    }
    if (fs::exists(entry.path() / "mt.cfg")) {
      return true;
    }
  }
  return false;
}

MutationResults Workspace::loadResults() const {
  MutationResults results;

  for (const auto& entry : fs::directory_iterator(mRoot)) {
    if (!fs::is_directory(entry.path())) {
      continue;
    }
    const std::string name = entry.path().filename().string();
    auto predicate = [](unsigned char c) { return std::isdigit(c) != 0; };
    if (name.empty() || !std::all_of(name.begin(), name.end(), predicate)) {
      continue;
    }

    fs::path donePath = entry.path() / "mt.done";
    if (!fs::exists(donePath)) {
      continue;
    }

    std::ifstream in(donePath);
    if (!in) {
      int id = std::stoi(name);
      throw std::runtime_error(fmt::format("Failed to read mt.done for mutant {}: {}", id, std::strerror(errno)));
    }
    MutationResult result;
    if (!(in >> result)) {
      int id = std::stoi(name);
      throw std::runtime_error(fmt::format("Failed to parse mt.done for mutant {}", id));
    }
    results.push_back(result);
  }

  return results;
}

std::vector<std::pair<int, Mutant>> Workspace::loadMutants() const {
  std::vector<std::pair<int, Mutant>> mutants;

  for (const auto& entry : fs::directory_iterator(mRoot)) {
    if (!fs::is_directory(entry.path())) {
      continue;
    }
    const std::string name = entry.path().filename().string();
    auto predicate = [](unsigned char c) { return std::isdigit(c) != 0; };
    if (name.empty() || !std::all_of(name.begin(), name.end(), predicate)) {
      continue;
    }

    int id = std::stoi(name);
    fs::path cfgPath = entry.path() / "mt.cfg";
    if (!fs::exists(cfgPath)) {
      continue;
    }

    std::ifstream in(cfgPath);
    Mutant m;
    in >> m;
    mutants.emplace_back(id, m);
  }

  std::sort(mutants.begin(), mutants.end(),
            [](const std::pair<int, Mutant>& a, const std::pair<int, Mutant>& b) { return a.first < b.first; });

  return mutants;
}


std::string Workspace::mutantDirName(int id) {
  // 5-digit zero-padded format supports IDs 1..kMaxMutantCount (99999).
  // If this format changes, update kMaxMutantCount in Workspace.hpp accordingly.
  return fmt::format("{:05d}", id);
}

fs::path Workspace::mutantFile(int id, const std::string& name) const {
  return getMutantDir(id) / name;
}

fs::path Workspace::getCompleteMarker() const {
  return mRoot / "run.done";
}

}  // namespace sentinel
