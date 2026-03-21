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

PopulateStage::PopulateStage(const Config& cfg, StatusLine& sl,
                             std::shared_ptr<Logger> log, fs::path workDir)
    : Stage(cfg, sl, std::move(log)), mWorkDir(std::move(workDir)) {}

bool PopulateStage::execute() {
  Workspace ws(mWorkDir);
  // Skip if mutants already written (resume or dry-run re-run)
  for (const auto& entry : fs::directory_iterator(mWorkDir)) {
    if (fs::is_directory(entry) &&
        entry.path().filename().string().find_first_not_of("0123456789") == std::string::npos) {
      if (fs::exists(entry.path() / "mt.cfg")) return true;  // already populated
    }
  }

  mStatusLine.setPhase(StatusLine::Phase::POPULATE);

  auto repo = std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions,
                                              *mConfig.patterns, *mConfig.excludes);
  repo->addSkipDir(mWorkDir);
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
  if (mConfig.partition && !mConfig.partition->empty()) {
    const std::string& s = *mConfig.partition;
    auto slash = s.find('/');
    partIdx = std::stoul(s.substr(0, slash));
    partCount = std::stoul(s.substr(slash + 1));
    std::size_t total = mutants.size();
    std::size_t start = (partIdx - 1) * total / partCount;
    std::size_t end   = partIdx * total / partCount;
    mutants = Mutants(mutants.begin() + static_cast<std::ptrdiff_t>(start),
                      mutants.begin() + static_cast<std::ptrdiff_t>(end));
  }

  int id = 1;
  for (auto& m : mutants) {
    ws.createMutant(id, m);
    id++;
  }

  WorkspaceStatus status;
  status.candidateCount = candidateCount;
  status.partIndex = partIdx;
  status.partCount = partCount;
  ws.saveStatus(status);

  return true;
}

}  // namespace sentinel
