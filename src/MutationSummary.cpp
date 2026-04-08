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
    totNumberOfTimeout(other.totNumberOfTimeout),
    timeByState(other.timeByState),
    totalBuildSecs(other.totalBuildSecs),
    totalTestSecs(other.totalTestSecs),
    timedMutantCount(other.timedMutantCount) {
  if (other.results.empty()) {
    return;
  }
  const MutationResult* base = &(*other.results.begin());
  // cppcheck-suppress unassignedVariable
  for (const auto& [path, fileStats] : other.groupByPath) {
    auto& entry = groupByPath[path];
    entry.total = fileStats.total;
    entry.detected = fileStats.detected;
    for (const auto* p : fileStats.results) {
      entry.results.push_back(&results.at(static_cast<std::size_t>(p - base)));
    }
  }
  // cppcheck-suppress unassignedVariable
  for (const auto& [dirPath, dirStats] : other.groupByDirPath) {
    auto& entry = groupByDirPath[dirPath];
    entry.total = dirStats.total;
    entry.detected = dirStats.detected;
    entry.fileCount = dirStats.fileCount;
    for (const auto* p : dirStats.results) {
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
  std::swap(timeByState, other.timeByState);
  std::swap(totalBuildSecs, other.totalBuildSecs);
  std::swap(totalTestSecs, other.totalTestSecs);
  std::swap(timedMutantCount, other.timedMutantCount);
  return *this;
}

void MutationSummary::aggregate() {
  for (const MutationResult& mr : results) {
    auto currentState = mr.getMutationState();

    auto& timing = timeByState[currentState];
    timing.count++;
    const double buildSecs = mr.getBuildSecs();
    const double testSecs = mr.getTestSecs();
    const bool hasTiming = (buildSecs > 0.0 || testSecs > 0.0);
    if (hasTiming) {
      timing.buildSecs += buildSecs;
      timing.testSecs += testSecs;
      timing.timedCount++;
      totalBuildSecs += buildSecs;
      totalTestSecs += testSecs;
      timedMutantCount++;
    }

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
    auto& dirStats = groupByDirPath[mrPath.parent_path()];
    auto& fileStats = groupByPath[mrPath];

    dirStats.results.push_back(&mr);
    dirStats.total++;

    fileStats.results.push_back(&mr);
    fileStats.total++;

    if (mr.getDetected()) {
      dirStats.detected++;
      fileStats.detected++;
      totNumberOfDetectedMutation++;
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
