/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <cctype>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
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

void Workspace::saveConfig(const std::string& yamlContent) {
  std::ofstream out(mRoot / "config.yaml");
  if (!out) {
    throw std::runtime_error(
        fmt::format("Failed to write workspace config: {}", (mRoot / "config.yaml").string()));
  }
  out << yamlContent;
}

void Workspace::saveStatus(const WorkspaceStatus& status) {
  WorkspaceStatus current;
  fs::path p = mRoot / "status.yaml";
  if (fs::exists(p)) {
    current = loadStatus();
  }
  if (status.baselineTime.has_value()) current.baselineTime = status.baselineTime;
  if (status.candidateCount.has_value()) current.candidateCount = status.candidateCount;
  if (status.partIndex.has_value()) current.partIndex = status.partIndex;
  if (status.partCount.has_value()) current.partCount = status.partCount;

  YAML::Emitter out;
  out << YAML::BeginMap;
  if (current.baselineTime.has_value()) {
    out << YAML::Key << "baseline-time" << YAML::Value << *current.baselineTime;
  }
  if (current.candidateCount.has_value()) {
    out << YAML::Key << "candidate-count" << YAML::Value << *current.candidateCount;
  }
  if (current.partIndex.has_value()) {
    out << YAML::Key << "part-index" << YAML::Value << *current.partIndex;
  }
  if (current.partCount.has_value()) {
    out << YAML::Key << "part-count" << YAML::Value << *current.partCount;
  }
  out << YAML::EndMap;

  std::ofstream f(p);
  if (!f) {
    throw std::runtime_error(fmt::format("Failed to write status.yaml: {}", p.string()));
  }
  f << out.c_str();
}

WorkspaceStatus Workspace::loadStatus() const {
  WorkspaceStatus status;
  fs::path p = mRoot / "status.yaml";
  if (!fs::exists(p)) return status;
  try {
    YAML::Node node = YAML::LoadFile(p.string());
    if (node["baseline-time"]) status.baselineTime = node["baseline-time"].as<std::size_t>();
    if (node["candidate-count"]) status.candidateCount = node["candidate-count"].as<std::size_t>();
    if (node["part-index"]) status.partIndex = node["part-index"].as<std::size_t>();
    if (node["part-count"]) status.partCount = node["part-count"].as<std::size_t>();
  } catch (const YAML::Exception& e) {
    throw std::runtime_error(fmt::format("Failed to read status.yaml: {}", e.what()));
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
    throw std::runtime_error(fmt::format("Failed to write mt.cfg for mutant {}", id));
  }
  out << m;
}

bool Workspace::isLocked(int id) const {
  return fs::exists(mutantFile(id, "mt.lock"));
}

void Workspace::setLock(int id) {
  std::ofstream f(mutantFile(id, "mt.lock"));
  if (!f) {
    throw std::runtime_error(fmt::format("Failed to create lock for mutant {}", id));
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
    throw std::runtime_error(fmt::format("Failed to create run.done in {}", mRoot.string()));
  }
}

void Workspace::setDone(int id, const MutationResult& result) {
  std::ofstream out(mutantFile(id, "mt.done"));
  if (!out) {
    throw std::runtime_error(fmt::format("Failed to write mt.done for mutant {}", id));
  }
  out << result;
}

MutationResult Workspace::getDoneResult(int id) const {
  std::ifstream in(mutantFile(id, "mt.done"));
  if (!in) {
    throw std::runtime_error(fmt::format("Cannot read mt.done for mutant {}", id));
  }
  MutationResult result;
  if (!(in >> result)) {
    throw std::runtime_error(fmt::format("Failed to parse mt.done for mutant {}", id));
  }
  return result;
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

void Workspace::copyTestReportTo(const fs::path& from, const fs::path& to,
                                 const std::vector<std::string>& exts) {
  fs::remove_all(to);
  fs::create_directories(to);
  if (fs::is_directory(from)) {
    for (const auto& dirent : fs::recursive_directory_iterator(from)) {
      if (dirent.is_regular_file()) {
        std::string ext = dirent.path().extension().string();
        if (ext.size() > 1) ext = ext.substr(1);
        if (exts.empty() || std::find(exts.begin(), exts.end(), ext) != exts.end()) {
          fs::copy(dirent.path(), to);
        }
      }
    }
  }
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
