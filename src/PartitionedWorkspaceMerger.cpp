/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/PartitionedWorkspaceMerger.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/version.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static const std::set<std::string> kExcludedConfigFields = {
    "source-dir", "compiledb-dir", "output-dir"};

static const std::vector<std::string> kMutantFiles = {
    "mt.cfg", "mt.done", "build.log", "test.log"};

PartitionedWorkspaceMerger::PartitionedWorkspaceMerger(
    const std::filesystem::path& targetDir,
    const std::vector<std::filesystem::path>& sourceDirs,
    bool force)
    : mTargetDir(targetDir), mSourceDirs(sourceDirs), mForce(force) {
}

void PartitionedWorkspaceMerger::merge() {
  for (const auto& source : mSourceDirs) {
    validateSource(source);
  }
  validateCompatibility();
  prepareTargetWorkspace();

  bool baselineCopied = fs::exists(mTargetDir / "config.yaml");
  const bool showProgress = mSourceDirs.size() > 1;
  std::size_t current = 0;
  for (const auto& source : mSourceDirs) {
    ++current;
    if (showProgress) {
      const auto& status = mStatusCache.at(source);
      Logger::info("Merging [{}/{}] partition {} from '{}'...",
                   current, mSourceDirs.size(),
                   *status.partIndex, source.string());
    }
    if (!baselineCopied) {
      copyBaseline(source);
      baselineCopied = true;
    }
    copyMutants(source);
  }

  updateCompleteness();
}

void PartitionedWorkspaceMerger::validateSource(
    const std::filesystem::path& sourceDir) const {
  if (!fs::exists(sourceDir / "config.yaml")) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is missing config.yaml.",
                    sourceDir.string()));
  }

  if (!fs::exists(sourceDir / "status.yaml")) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is missing status.yaml.",
                    sourceDir.string()));
  }

  const Workspace ws(sourceDir);
  auto status = ws.loadStatus();
  if (!status.partIndex.has_value() || !status.partCount.has_value() ||
      *status.partIndex == 0 || *status.partCount == 0) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is not a partition run.",
                    sourceDir.string()));
  }

  if (!status.candidateCount.has_value()) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is missing candidate-count "
                    "in status.yaml.",
                    sourceDir.string()));
  }

  if (!status.seed.has_value()) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is missing seed in status.yaml.",
                    sourceDir.string()));
  }

  if (!status.version.has_value()) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is missing version in status.yaml.",
                    sourceDir.string()));
  }

  if (*status.version != PROGRAM_VERSION) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' has version {} but "
                    "current program version is {}.",
                    sourceDir.string(), *status.version, PROGRAM_VERSION));
  }

  if (!ws.isComplete()) {
    throw std::runtime_error(
        fmt::format("Source workspace '{}' is not complete "
                    "(run.done missing).",
                    sourceDir.string()));
  }

  mStatusCache.emplace(sourceDir, std::move(status));
}

std::string PartitionedWorkspaceMerger::loadConfigWithoutExcludedFields(
    const std::filesystem::path& sourceDir) {
  YAML::Node node = YAML::LoadFile((sourceDir / "config.yaml").string());
  for (const auto& field : kExcludedConfigFields) {
    node.remove(field);
  }

  YAML::Emitter emitter;
  emitter << node;
  return emitter.c_str();
}

void PartitionedWorkspaceMerger::validateCompatibility() const {
  const auto& firstStatus = mStatusCache.at(mSourceDirs[0]);
  const std::string firstConfig =
      loadConfigWithoutExcludedFields(mSourceDirs[0]);

  std::set<std::size_t> seenIndices;

  // Check against already-merged partitions in target workspace
  if (fs::exists(mTargetDir / "status.yaml")) {
    const Workspace targetWs(mTargetDir);
    const auto targetStatus = targetWs.loadStatus();
    if (targetStatus.mergedPartitions.has_value()) {
      for (const auto idx : *targetStatus.mergedPartitions) {
        seenIndices.insert(idx);
      }
    }
    if (targetStatus.partCount.has_value() &&
        *targetStatus.partCount != *firstStatus.partCount) {
      throw std::runtime_error(
          fmt::format("part-count mismatch: target has {} but "
                      "source '{}' has {}.",
                      *targetStatus.partCount,
                      mSourceDirs[0].string(),
                      *firstStatus.partCount));
    }
    if (targetStatus.candidateCount.has_value() &&
        *targetStatus.candidateCount != *firstStatus.candidateCount) {
      throw std::runtime_error(
          fmt::format("candidate-count mismatch: target has {} but "
                      "source '{}' has {}.",
                      *targetStatus.candidateCount,
                      mSourceDirs[0].string(),
                      *firstStatus.candidateCount));
    }
    if (targetStatus.seed.has_value() &&
        *targetStatus.seed != *firstStatus.seed) {
      throw std::runtime_error(
          fmt::format("seed mismatch: target has {} but source '{}' has {}.",
                      *targetStatus.seed,
                      mSourceDirs[0].string(),
                      *firstStatus.seed));
    }
    if (targetStatus.version.has_value() &&
        *targetStatus.version != *firstStatus.version) {
      throw std::runtime_error(
          fmt::format("version mismatch: target has {} but source '{}' has {}.",
                      *targetStatus.version,
                      mSourceDirs[0].string(),
                      *firstStatus.version));
    }
  }

  if (!seenIndices.insert(*firstStatus.partIndex).second) {
    throw std::runtime_error(
        fmt::format("Duplicate part-index {} from '{}'.",
                    *firstStatus.partIndex, mSourceDirs[0].string()));
  }

  for (std::size_t i = 1; i < mSourceDirs.size(); ++i) {
    const auto& status = mStatusCache.at(mSourceDirs[i]);

    if (*status.partCount != *firstStatus.partCount) {
      throw std::runtime_error(
          fmt::format("part-count mismatch: '{}' has {} but '{}' has {}.",
                      mSourceDirs[0].string(), *firstStatus.partCount,
                      mSourceDirs[i].string(), *status.partCount));
    }

    if (*status.candidateCount != *firstStatus.candidateCount) {
      throw std::runtime_error(
          fmt::format("candidate-count mismatch: '{}' has {} but "
                      "'{}' has {}.",
                      mSourceDirs[0].string(), *firstStatus.candidateCount,
                      mSourceDirs[i].string(), *status.candidateCount));
    }

    if (*status.seed != *firstStatus.seed) {
      throw std::runtime_error(
          fmt::format("seed mismatch: '{}' has {} but '{}' has {}.",
                      mSourceDirs[0].string(), *firstStatus.seed,
                      mSourceDirs[i].string(), *status.seed));
    }

    if (!seenIndices.insert(*status.partIndex).second) {
      throw std::runtime_error(
          fmt::format("Duplicate part-index {} from '{}'.",
                      *status.partIndex, mSourceDirs[i].string()));
    }

    const std::string thisConfig = loadConfigWithoutExcludedFields(mSourceDirs[i]);
    if (thisConfig != firstConfig) {
      throw std::runtime_error(
          fmt::format("Config mismatch between '{}' and '{}'.",
                      mSourceDirs[0].string(), mSourceDirs[i].string()));
    }
  }
}

void PartitionedWorkspaceMerger::prepareTargetWorkspace() const {
  if (!fs::exists(mTargetDir)) {
    fs::create_directories(mTargetDir);
  }
}

void PartitionedWorkspaceMerger::copyBaseline(
    const std::filesystem::path& sourceDir) const {
  copyFileWithConflictCheck(sourceDir / "config.yaml",
                            mTargetDir / "config.yaml");

  const fs::path srcOriginal = sourceDir / "original";
  const fs::path dstOriginal = mTargetDir / "original";
  fs::create_directories(dstOriginal / "results");

  for (const auto& entry : fs::recursive_directory_iterator(srcOriginal)) {
    if (!fs::is_regular_file(entry.path())) {
      continue;
    }
    const fs::path rel = entry.path().lexically_relative(srcOriginal);
    const fs::path dst = dstOriginal / rel;
    fs::create_directories(dst.parent_path());
    copyFileWithConflictCheck(entry.path(), dst);
  }
}

void PartitionedWorkspaceMerger::copyMutants(
    const std::filesystem::path& sourceDir) const {
  const Workspace srcWs(sourceDir);
  const Workspace targetWs(mTargetDir);
  const auto mutants = srcWs.loadMutants();

  for (const auto& [id, mutant] : mutants) {
    if (!srcWs.isDone(id)) {
      continue;
    }

    const fs::path srcDir = srcWs.getMutantDir(id);
    const fs::path dstDir = targetWs.getMutantDir(id);
    fs::create_directories(dstDir);

    for (const auto& name : kMutantFiles) {
      const fs::path srcFile = srcDir / name;
      if (fs::exists(srcFile)) {
        copyFileWithConflictCheck(srcFile, dstDir / name);
      }
    }
  }
}

std::string PartitionedWorkspaceMerger::readFileContent(
    const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error(
        fmt::format("Failed to read file: '{}'", path.string()));
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

void PartitionedWorkspaceMerger::copyFileWithConflictCheck(
    const std::filesystem::path& src,
    const std::filesystem::path& dst) const {
  if (!fs::exists(dst)) {
    fs::copy_file(src, dst);
    return;
  }

  if (fs::file_size(src) != fs::file_size(dst)) {
    throw std::runtime_error(
        fmt::format("Conflict: '{}' exists with different content. "
                    "Cannot merge.",
                    dst.string()));
  }

  const std::string srcContent = readFileContent(src);
  const std::string dstContent = readFileContent(dst);

  if (srcContent == dstContent) {
    if (mForce) {
      return;
    }
    throw std::runtime_error(
        fmt::format("File already exists: '{}'. "
                    "Use --force to overwrite identical files.",
                    dst.string()));
  }

  throw std::runtime_error(
      fmt::format("Conflict: '{}' exists with different content. "
                  "Cannot merge.",
                  dst.string()));
}

void PartitionedWorkspaceMerger::updateCompleteness() const {
  Workspace targetWs(mTargetDir);
  const auto& firstStatus = mStatusCache.at(mSourceDirs[0]);
  const std::size_t partCount = *firstStatus.partCount;

  std::vector<std::size_t> merged;
  if (fs::exists(mTargetDir / "status.yaml")) {
    const auto targetStatus = targetWs.loadStatus();
    if (targetStatus.mergedPartitions.has_value()) {
      merged = *targetStatus.mergedPartitions;
    }
  }

  for (const auto& source : mSourceDirs) {
    const auto& status = mStatusCache.at(source);
    merged.push_back(*status.partIndex);
  }

  std::sort(merged.begin(), merged.end());

  WorkspaceStatus newStatus;
  newStatus.version = firstStatus.version;
  newStatus.seed = firstStatus.seed;
  newStatus.candidateCount = firstStatus.candidateCount;
  newStatus.partCount = partCount;
  newStatus.mergedPartitions = merged;
  targetWs.saveStatus(newStatus);

  const bool complete = merged.size() == partCount;

  if (complete) {
    targetWs.setComplete();
    Logger::info("All partitions collected ({}/{}). Merge complete.",
                 merged.size(), partCount);
  } else {
    std::vector<std::size_t> missing;
    for (std::size_t i = 1; i <= partCount; ++i) {
      if (!std::binary_search(merged.begin(), merged.end(), i)) {
        missing.push_back(i);
      }
    }
    Logger::info("Collected partitions {}/{}, missing: {}.",
                 merged.size(), partCount, fmt::join(missing, ", "));
  }
}

}  // namespace sentinel
