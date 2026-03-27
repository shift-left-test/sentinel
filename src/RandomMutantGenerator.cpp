/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <filesystem>  // NOLINT
#include <map>
#include <random>
#include <set>
#include <utility>
#include <vector>
#include "sentinel/RandomMutantGenerator.hpp"

namespace sentinel {

namespace fs = std::filesystem;

RandomMutantGenerator::RandomMutantGenerator(const std::filesystem::path& path) : MutantGenerator(path) {
}

Mutants RandomMutantGenerator::selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                                             unsigned int randomSeed, const CandidateIndex& index) {
  std::mt19937 rng(randomSeed);
  std::map<fs::path, fs::path> pathCache;
  std::set<Mutant> selectedSet;
  Mutants candidates;
  std::vector<const Mutant*> lineCandidates;

  for (const auto& line : sourceLines) {
    fs::path rawPath = line.getPath();
    auto emplaceResult = pathCache.emplace(rawPath, fs::path{});
    if (emplaceResult.second) {
      emplaceResult.first->second = fs::canonical(rawPath);
    }
    const fs::path& canonPath = emplaceResult.first->second;

    findCandidatesForLine(index, canonPath, line.getLineNumber(), &lineCandidates);
    if (lineCandidates.empty()) {
      continue;
    }
    mLinesByPath[canonPath]++;
    std::shuffle(lineCandidates.begin(), lineCandidates.end(), rng);
    for (const auto* candidate : lineCandidates) {
      if (selectedSet.insert(*candidate).second) {
        candidates.push_back(*candidate);
        break;
      }
    }
  }

  std::size_t n = candidates.size();
  mCandidateCount = n;
  if (maxMutants == 0 || n <= maxMutants) {
    return candidates;
  }

  // Partial Fisher-Yates: select maxMutants from candidates pool.
  for (std::size_t i = 0; i < maxMutants; ++i) {
    std::uniform_int_distribution<std::size_t> dist(i, n - 1);
    std::size_t j = dist(rng);
    std::swap(candidates[i], candidates[j]);
  }
  return Mutants(candidates.begin(), candidates.begin() + maxMutants);
}

}  // namespace sentinel
