/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Workspace.hpp"

namespace fs = std::filesystem;

namespace sentinel {

Workspace::Workspace(const fs::path& root) : mRoot(root) {}

bool Workspace::hasPreviousRun() const {
  return fs::exists(mRoot / "sentinel.yaml");
}

void Workspace::initialize() {
  fs::remove_all(mRoot);
  fs::create_directories(mRoot / "original" / "results");
  fs::create_directories(mRoot / "backup");
}

void Workspace::saveConfig(const std::string& yamlContent) {
  std::ofstream out(mRoot / "sentinel.yaml");
  if (!out) {
    throw std::runtime_error(
        fmt::format("Failed to write workspace config: {}", (mRoot / "sentinel.yaml").string()));
  }
  out << yamlContent;
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

fs::path Workspace::getMutantDir(int id) const {
  return mRoot / mutantDirName(id);
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

std::string Workspace::mutantDirName(int id) {
  return fmt::format("{:05d}", id);
}

fs::path Workspace::mutantFile(int id, const std::string& name) const {
  return getMutantDir(id) / name;
}

}  // namespace sentinel
