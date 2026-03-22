/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/GitRepository.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/PopulateStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

PopulateStage::PopulateStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                             std::shared_ptr<Logger> log,
                             std::shared_ptr<Workspace> workspace)
    : Stage(cfg, std::move(sl), std::move(log)), mWorkspace(std::move(workspace)) {}

bool PopulateStage::execute() {
  // Skip if mutants already written (resume or dry-run re-run)
  if (!mWorkspace->loadMutants().empty()) return true;

  mStatusLine->setPhase(StatusLine::Phase::POPULATE);

  auto repo = std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions,
                                              *mConfig.patterns, *mConfig.excludes);
  repo->addSkipDir(mWorkspace->getRoot());
  SourceLines sourceLines = repo->getSourceLines(*mConfig.scope);

  unsigned int seed = mConfig.seed ? *mConfig.seed : std::random_device {}();
  std::shuffle(sourceLines.begin(), sourceLines.end(), std::mt19937(seed));

  auto generator = MutantGenerator::getInstance(*mConfig.generator, *mConfig.compileDbDir);
  generator->setOperators(*mConfig.operators);
  MutationFactory factory(generator);
  auto mutants = factory.populate(*mConfig.sourceDir, sourceLines, *mConfig.limit, seed,
                                  *mConfig.generator);
  std::size_t candidateCount = generator->getCandidateCount();

  if (mutants.size() > static_cast<std::size_t>(Workspace::kMaxMutantCount)) {
    throw std::runtime_error(fmt::format(
        "Too many mutants: {} generated, maximum is {}. "
        "Use --limit to reduce the number of mutants.",
        mutants.size(), Workspace::kMaxMutantCount));
  }

  // Apply partition slice
  std::size_t partIdx = 0;
  std::size_t partCount = 0;
  std::size_t partStart = 0;
  if (mConfig.partition && !mConfig.partition->empty()) {
    const std::string& s = *mConfig.partition;
    auto slash = s.find('/');
    partIdx = std::stoul(s.substr(0, slash));
    partCount = std::stoul(s.substr(slash + 1));
    std::size_t total = mutants.size();
    partStart = (partIdx - 1) * total / partCount;
    std::size_t end = partIdx * total / partCount;
    mutants = Mutants(mutants.begin() + static_cast<std::ptrdiff_t>(partStart),
                      mutants.begin() + static_cast<std::ptrdiff_t>(end));
  }

  int id = static_cast<int>(partStart) + 1;
  for (auto& m : mutants) {
    mWorkspace->createMutant(id, m);
    id++;
  }

  WorkspaceStatus status;
  status.candidateCount = candidateCount;
  status.partIndex = partIdx;
  status.partCount = partCount;
  mWorkspace->saveStatus(status);

  return true;
}

}  // namespace sentinel
