/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>  // NOLINT
#include <set>
#include <string>
#include <utility>
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/formatter.hpp"

namespace sentinel {

namespace fs = std::filesystem;

MutationSummary::MutationSummary(const MutationResults& r, const std::filesystem::path& src) :
    results(r), sourcePath(src) {
  if (!fs::is_directory(sourcePath)) {
    throw InvalidArgumentException(fmt::format("source path does not exist: {}", sourcePath));
  }
  aggregate();
}

MutationSummary::MutationSummary(const std::filesystem::path& resultsPath, const std::filesystem::path& src) :
    sourcePath(src) {
  if (fs::exists(resultsPath) && !fs::is_regular_file(resultsPath)) {
    throw InvalidArgumentException(fmt::format("results path is not a regular file: {}", resultsPath));
  }
  if (!fs::is_directory(sourcePath)) {
    throw InvalidArgumentException(fmt::format("source path does not exist: {}", sourcePath));
  }
  results.load(resultsPath.string());
  aggregate();
}

MutationSummary::MutationSummary(const MutationSummary& other) :
    results(other.results),
    sourcePath(other.sourcePath),
    totNumberOfMutation(other.totNumberOfMutation),
    totNumberOfDetectedMutation(other.totNumberOfDetectedMutation),
    totNumberOfBuildFailure(other.totNumberOfBuildFailure),
    totNumberOfRuntimeError(other.totNumberOfRuntimeError),
    totNumberOfTimeout(other.totNumberOfTimeout) {
  if (other.results.size() == 0) {
    return;
  }
  const MutationResult* base = &(*other.results.begin());
  for (const auto& [k, v] : other.groupByPath) {
    auto& entry = groupByPath[k];
    entry.total = v.total;
    entry.detected = v.detected;
    for (const auto* p : v.results) {
      entry.results.push_back(&results.at(static_cast<std::size_t>(p - base)));
    }
  }
  for (const auto& [k, v] : other.groupByDirPath) {
    auto& entry = groupByDirPath[k];
    entry.total = v.total;
    entry.detected = v.detected;
    entry.fileCount = v.fileCount;
    for (const auto* p : v.results) {
      entry.results.push_back(&results.at(static_cast<std::size_t>(p - base)));
    }
  }
}

MutationSummary& MutationSummary::operator=(MutationSummary other) {
  std::swap(results, other.results);
  std::swap(sourcePath, other.sourcePath);
  std::swap(groupByDirPath, other.groupByDirPath);
  std::swap(groupByPath, other.groupByPath);
  std::swap(totNumberOfMutation, other.totNumberOfMutation);
  std::swap(totNumberOfDetectedMutation, other.totNumberOfDetectedMutation);
  std::swap(totNumberOfBuildFailure, other.totNumberOfBuildFailure);
  std::swap(totNumberOfRuntimeError, other.totNumberOfRuntimeError);
  std::swap(totNumberOfTimeout, other.totNumberOfTimeout);
  return *this;
}

void MutationSummary::aggregate() {
  for (const MutationResult& mr : results) {
    auto currentState = mr.getMutationState();
    if (currentState == MutationState::BUILD_FAILURE) {
      totNumberOfBuildFailure++;
      continue;
    }
    if (currentState == MutationState::RUNTIME_ERROR) {
      totNumberOfRuntimeError++;
      continue;
    }
    if (currentState == MutationState::TIMEOUT) {
      totNumberOfTimeout++;
      continue;
    }
    totNumberOfMutation++;

    fs::path mrPath = mr.getMutant().getPath();
    fs::path curDirpath = mrPath.parent_path();

    groupByDirPath[curDirpath].results.push_back(&mr);
    groupByDirPath[curDirpath].total += 1;

    groupByPath[mrPath].results.push_back(&mr);
    groupByPath[mrPath].total += 1;

    if (mr.getDetected()) {
      groupByDirPath[curDirpath].detected += 1;
      groupByPath[mrPath].detected += 1;
      totNumberOfDetectedMutation += 1;
    }
  }

  for (auto& p : groupByDirPath) {
    std::set<fs::path> tmpSet;
    for (const MutationResult* mr : p.second.results) {
      tmpSet.insert(mr->getMutant().getPath());
    }
    p.second.fileCount = tmpSet.size();
  }
}

}  // namespace sentinel
