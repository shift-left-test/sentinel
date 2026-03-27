/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <filesystem>  // NOLINT
#include <map>
#include <random>
#include <set>
#include <vector>
#include "sentinel/UniformMutantGenerator.hpp"

namespace sentinel {

namespace fs = std::filesystem;

UniformMutantGenerator::UniformMutantGenerator(const std::filesystem::path& path) : MutantGenerator(path) {
}

Mutants UniformMutantGenerator::selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                                              unsigned int randomSeed, const CandidateIndex& index) {
  std::set<Mutant> selectedSet;
  Mutants result;
  std::mt19937 rng(randomSeed);
  std::map<fs::path, fs::path> pathCache;
  std::size_t candidateLineCount = 0;
  std::vector<const Mutant*> candidates;

  for (const auto& line : sourceLines) {
    fs::path rawPath = line.getPath();
    auto emplaceResult = pathCache.emplace(rawPath, fs::path{});
    if (emplaceResult.second) {
      emplaceResult.first->second = fs::canonical(rawPath);
    }
    const fs::path& canonPath = emplaceResult.first->second;

    findCandidatesForLine(index, canonPath, line.getLineNumber(), &candidates);
    if (candidates.empty()) {
      continue;
    }

    candidateLineCount++;
    mLinesByPath[canonPath]++;

    if (maxMutants > 0 && result.size() == maxMutants) {
      continue;
    }

    std::shuffle(candidates.begin(), candidates.end(), rng);
    for (const auto* candidate : candidates) {
      if (selectedSet.insert(*candidate).second) {
        result.push_back(*candidate);
        break;
      }
    }
  }

  mCandidateCount = candidateLineCount;
  return result;
}

}  // namespace sentinel
