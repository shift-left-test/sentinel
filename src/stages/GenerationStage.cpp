/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/GenerationStage.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kSummaryWidth = 76;

/**
 * @brief Fills a UTF-8 repeated-character line of the given display width.
 */
static std::string repeatUtf8(const char* ch, std::size_t width) {
  std::string unit(ch);
  std::string result;
  result.reserve(unit.size() * width);
  for (std::size_t i = 0; i < width; ++i) {
    result += unit;
  }
  return result;
}

static void printGenerationSummary(const Mutants& mutants, std::size_t candidateCount,
                                   const fs::path& sourceDir, const std::string& generatorStr,
                                   unsigned seed, const std::string& partition) {
  const std::string thick = repeatUtf8("\xe2\x94\x81", kSummaryWidth);   // ━
  const std::string thin = repeatUtf8("\xe2\x94\x80", kSummaryWidth);    // ─

  std::size_t flen = kSummaryWidth - 12;
  std::size_t mlen = 10;
  std::string rowFmt = "  {0:<{1}}{2:>{3}}";

  // Count mutants per file and per operator
  std::map<fs::path, std::size_t> groupByPath;
  std::map<std::string, std::size_t> groupByOperator;
  auto root = fs::canonical(sourceDir);
  for (const auto& m : mutants) {
    auto file = fs::canonical(m.getPath());
    groupByPath[file.lexically_relative(root)]++;
    groupByOperator[m.getOperator()]++;
  }

  // Header
  Console::out("{}", thick);
  Console::out("{:^{}}", "Mutant Generation Summary", kSummaryWidth);
  Console::out("{}", thick);

  // Summary
  Console::out("  Selected:   {} of {} candidates from {} file{}",
               mutants.size(), candidateCount, groupByPath.size(),
               groupByPath.size() == 1 ? "" : "s");
  Console::out("  Generator:  {} (seed: {})", generatorStr, seed);
  if (!partition.empty()) {
    Console::out("  Partition:  {}", partition);
  }

  // File table
  Console::out("{}", thin);
  Console::out(rowFmt, "File", flen, "Mutants", mlen);
  Console::out("{}", thin);
  for (const auto& [path, count] : groupByPath) {
    std::string filePath = path.string();
    int overflow = static_cast<int>(filePath.size()) - static_cast<int>(flen);
    std::string skipStr;
    if (overflow > 1) {
      filePath = filePath.substr(overflow + 4);
      skipStr = "... ";
    }
    Console::out(rowFmt, skipStr + filePath, flen, count, mlen);
  }

  // Operator table
  Console::out("{}", thin);
  Console::out(rowFmt, "Operator", flen, "Mutants", mlen);
  Console::out("{}", thin);
  for (const auto& [op, count] : groupByOperator) {
    Console::out(rowFmt, op, flen, count, mlen);
  }

  // Footer
  Console::out("{}", thick);
}

GenerationStage::GenerationStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                                 std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool GenerationStage::shouldSkip() const {
  return !mWorkspace->loadMutants().empty();
}

StatusLine::Phase GenerationStage::getPhase() const {
  return StatusLine::Phase::GENERATION;
}

bool GenerationStage::execute() {
  Logger::info("Generating mutants...");
  auto repo =
      std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions, *mConfig.patterns, *mConfig.excludes);
  repo->addSkipDir(mWorkspace->getRoot());
  SourceLines sourceLines = repo->getSourceLines(*mConfig.scope);

  unsigned int seed = mConfig.seed ? *mConfig.seed : std::random_device {}();
  std::shuffle(sourceLines.begin(), sourceLines.end(), std::mt19937(seed));

  auto generator = MutantGenerator::getInstance(*mConfig.generator, *mConfig.compileDbDir);
  generator->setOperators(*mConfig.operators);
  MutationFactory factory(generator);
  auto mutants = factory.generate(*mConfig.sourceDir, sourceLines, *mConfig.limit, seed);
  std::size_t candidateCount = generator->getCandidateCount();

  if (mutants.size() > static_cast<std::size_t>(Workspace::kMaxMutantCount)) {
    throw std::runtime_error(
        fmt::format("Too many mutants: {} generated, maximum is {}. "
                    "Use --limit to reduce the number of mutants.",
                    mutants.size(), Workspace::kMaxMutantCount));
  }

  // Print summary before partition (shows full generation results)
  std::string partition = (mConfig.partition && !mConfig.partition->empty()) ? *mConfig.partition : "";
  printGenerationSummary(mutants, candidateCount, *mConfig.sourceDir, *mConfig.generator, seed, partition);

  // Apply partition slice
  std::size_t partIdx = 0;
  std::size_t partCount = 0;
  std::size_t partStart = 0;
  if (!partition.empty()) {
    auto part = Partition::parse(partition);
    partIdx = part.index;
    partCount = part.count;
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
