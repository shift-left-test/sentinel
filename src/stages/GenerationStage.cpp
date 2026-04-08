/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include "sentinel/Console.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/operators/MutationOperatorExpansion.hpp"
#include "sentinel/stages/GenerationStage.hpp"
#include "sentinel/util/Utf8Char.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr std::size_t kSummaryWidth = 80;
static constexpr std::size_t kMutantsCol = 13;
static constexpr std::size_t kLinesCol = 9;


static void printGenerationSummary(const Mutants& mutants, std::size_t candidateCount,
                                   const std::map<fs::path, std::size_t>& linesByPath,
                                   const fs::path& sourceDir, Generator generator,
                                   unsigned seed, std::size_t limit,
                                   const std::optional<std::string>& from,
                                   bool uncommitted,
                                   const std::string& partition) {
  const std::string thick = Utf8Char::ThickLine * kSummaryWidth;
  const std::string thin = Utf8Char::ThinLine * kSummaryWidth;

  std::size_t flen = kSummaryWidth - kMutantsCol - kLinesCol - 2;

  std::string rowFmt2 = "  {0:<{1}}{2:>{3}}{4:>{5}}";
  std::string rowFmt1 = "  {0:<{1}}{2:>{3}}";

  // Count mutants per file and per operator
  std::map<fs::path, std::size_t> groupByPath;
  std::map<std::string, std::size_t> groupByOperator;
  for (const auto& m : mutants) {
    groupByPath[m.getPath()]++;
    groupByOperator[m.getOperator()]++;
  }

  // Build lines-by-relative-path map
  const auto root = fs::canonical(sourceDir);
  std::map<fs::path, std::size_t> linesByRelPath;
  for (const auto& [absPath, count] : linesByPath) {
    linesByRelPath[absPath.lexically_relative(root)] = count;
  }

  // Header
  Console::out("{}", thick);
  Console::out("{:^{}}", "Mutant Generation Summary", kSummaryWidth);
  Console::out("{}", thick);

  // Summary
  if (from && uncommitted) {
    Console::out("  Target:     changes from {} + uncommitted ({} file{}, {} lines)",
                 *from, groupByPath.size(),
                 groupByPath.size() == 1 ? "" : "s", candidateCount);
  } else if (from) {
    Console::out("  Target:     changes from {} ({} file{}, {} lines)",
                 *from, groupByPath.size(),
                 groupByPath.size() == 1 ? "" : "s", candidateCount);
  } else if (uncommitted) {
    Console::out("  Target:     uncommitted changes ({} file{}, {} lines)",
                 groupByPath.size(),
                 groupByPath.size() == 1 ? "" : "s", candidateCount);
  } else {
    Console::out("  Target:     all sources ({} file{}, {} lines)",
                 groupByPath.size(),
                 groupByPath.size() == 1 ? "" : "s", candidateCount);
  }
  Console::out("  Generator:  {} (seed: {})", generatorToString(generator), seed);
  std::string mutantsLine = fmt::format("{}", mutants.size());
  if (limit > 0) {
    mutantsLine += fmt::format(" of {} limit", limit);
  }
  if (!partition.empty()) {
    mutantsLine += fmt::format(" (partition: {})", partition);
  }
  Console::out("  Mutants:    {}", mutantsLine);

  // File table
  Console::out("{}", thin);
  Console::out(rowFmt2, "File", flen, "Mutants", kMutantsCol, "Lines", kLinesCol);
  Console::out("{}", thin);
  for (const auto& [path, count] : groupByPath) {
    std::string filePath = string::truncate(path.string(), flen);
    std::size_t lines = 0;
    auto it = linesByRelPath.find(path);
    if (it != linesByRelPath.end()) {
      lines = it->second;
    }
    Console::out(rowFmt2, filePath, flen, count, kMutantsCol, lines, kLinesCol);
  }

  // Operator table
  Console::out("{}", thin);
  Console::out("  Operator");
  Console::out("{}", thin);
  for (const auto& [op, count] : groupByOperator) {
    Console::out(rowFmt1, mutationOperatorToExpansion(op), flen, count, kMutantsCol);
  }

  // Footer
  Console::out("{}", thick);
}

GenerationStage::GenerationStage(std::shared_ptr<GitRepository> repo,
                                 std::shared_ptr<MutantGenerator> generator) :
    mRepo(std::move(repo)), mGenerator(std::move(generator)) {
}

bool GenerationStage::shouldSkip(const PipelineContext& ctx) const {
  return ctx.workspace.hasMutants();
}

StatusLine::Phase GenerationStage::getPhase() const {
  return StatusLine::Phase::GENERATION;
}

bool GenerationStage::execute(PipelineContext* ctx) {
  Logger::info("Generating mutants...");
  mRepo->addSkipDir(ctx->workspace.getRoot());
  SourceLines sourceLines = mRepo->getSourceLines(ctx->config.from, ctx->config.uncommitted);

  // Verbose: source scan results and config
  if (isVerbose(*ctx)) {
    std::set<fs::path> uniqueFiles;
    for (const auto& sl : sourceLines) {
      uniqueFiles.insert(sl.getPath());
    }
    Logger::verbose("Source: {} lines from {} files", sourceLines.size(), uniqueFiles.size());
    Logger::verbose("Extensions: {}", fmt::join(ctx->config.extensions, ", "));
    if (!ctx->config.patterns.empty()) {
      Logger::verbose("Patterns: {}", fmt::join(ctx->config.patterns, ", "));
    }
  }

  unsigned int seed = ctx->config.seed ? *ctx->config.seed : std::random_device {}();
  std::shuffle(sourceLines.begin(), sourceLines.end(), std::mt19937(seed));

  mGenerator->setOperators(ctx->config.operators);
  MutationFactory factory(mGenerator);
  auto mutants = factory.generate(ctx->config.sourceDir, sourceLines, ctx->config.limit, seed);
  std::size_t candidateCount = mGenerator->getCandidateCount();

  if (mutants.size() > static_cast<std::size_t>(Workspace::kMaxMutantCount)) {
    throw std::runtime_error(
        fmt::format("Too many mutants: {} generated, maximum is {}. "
                    "Use --limit to reduce the number of mutants.",
                    mutants.size(), Workspace::kMaxMutantCount));
  }

  // Print summary before partition (shows full generation results)
  std::string partition = ctx->config.partition.value_or("");
  auto linesByPath = mGenerator->getLinesByPath();
  printGenerationSummary(mutants, candidateCount, linesByPath, ctx->config.sourceDir, ctx->config.generator, seed,
                         ctx->config.limit, ctx->config.from, ctx->config.uncommitted, partition);

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
    ctx->workspace.createMutant(id, m);
    id++;
  }

  WorkspaceStatus status;
  status.seed = seed;
  status.candidateCount = candidateCount;
  status.partIndex = partIdx;
  status.partCount = partCount;
  ctx->workspace.saveStatus(status);

  return true;
}

}  // namespace sentinel
