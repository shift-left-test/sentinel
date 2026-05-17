/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include "sentinel/MutationFactory.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

MutationFactory::MutationFactory(const std::shared_ptr<MutantGenerator>& generator) : mGenerator(generator) {
}

Mutants MutationFactory::generate(const std::filesystem::path& gitPath, const SourceLines& sourceLines,
                                  std::size_t maxMutants, unsigned int randomSeed, std::size_t mutantsPerLine) {
  auto mutants = mGenerator->generate(sourceLines, maxMutants, randomSeed, mutantsPerLine);
  // gitPath is the project's source root; failing to canonicalize it is
  // structural and should surface with a clear message rather than the raw
  // std::filesystem error.
  std::error_code rootEc;
  const auto root = fs::canonical(gitPath, rootEc);
  if (rootEc) {
    throw IOException(rootEc.value(), fmt::format(
        "Cannot canonicalize source root '{}': {}",
        gitPath.string(), rootEc.message()));
  }
  std::map<fs::path, fs::path> canonCache;
  for (auto& m : mutants) {
    auto [it, inserted] = canonCache.emplace(m.getPath(), fs::path{});
    if (inserted) {
      // Per-mutant canonical fall-back: if a source file was removed between
      // AST collection and this loop (rare race), keep the original path so
      // the run can continue with that mutant's relative path still derived
      // from the unresolved input.
      std::error_code ec;
      it->second = fs::canonical(m.getPath(), ec);
      if (ec) {
        it->second = m.getPath();
      }
    }
    const auto& absPath = it->second;
    m = Mutant(m.getOperator(), absPath.lexically_relative(root), m.getQualifiedFunction(),
               m.getFirst().line, m.getFirst().column,
               m.getLast().line, m.getLast().column, m.getToken());
  }
  return mutants;
}

}  // namespace sentinel
