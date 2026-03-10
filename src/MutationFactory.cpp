/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT(build/c++17)
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/util/string.hpp"

namespace fs = std::filesystem;

namespace sentinel {

MutationFactory::MutationFactory(const std::shared_ptr<MutantGenerator>& generator) : mGenerator(generator) {}

Mutants MutationFactory::populate(const std::string& gitPath, const SourceLines& sourceLines, std::size_t maxMutants,
                                  unsigned randomSeed, const std::string& generatorStr) {
  auto logger = Logger::getLogger("populate");
  logger->debug("random seed: {}", randomSeed);
  Mutants mutables = mGenerator->populate(sourceLines, maxMutants, randomSeed);

  // Count mutants per file and per operator
  std::map<std::string, std::size_t> groupByPath;
  std::map<std::string, std::size_t> groupByOperator;
  for (const auto& m : mutables) {
    groupByPath[m.getPath()]++;
    groupByOperator[m.getOperator()]++;
  }

  std::size_t flen = 50;
  std::size_t mlen = 10;
  std::size_t maxlen = flen + mlen + 2;
  std::string defFormat = "{0:<{1}}{2:>{3}}\n";

  // ── File table ──────────────────────────────────────────────────────────────
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);
  std::cout << string::rtrim(fmt::format("{0:^{1}}", "Mutant Population Report", maxlen)) << "\n";
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat, "File", flen, "Mutants", mlen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  for (const auto& p : groupByPath) {
    auto root = fs::canonical(gitPath);
    auto file = fs::canonical(p.first);
    std::string filePath = file.lexically_relative(root).string();

    int filePos = filePath.size() - flen;
    std::string skipStr;
    if (filePos < 0) {
      filePos = 0;
    } else if (filePos > 1) {
      filePos += 4;
      skipStr = "... ";
    }
    std::cout << fmt::format(defFormat, skipStr + filePath.substr(filePos), flen, p.second, mlen);
  }

  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat, "TOTAL", flen, mutables.size(), mlen);
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);

  // ── Operator table ──────────────────────────────────────────────────────────
  std::cout << "\n";
  std::cout << fmt::format(defFormat, "Operator", flen, "Mutants", mlen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  for (const auto& p : groupByOperator) {
    std::cout << fmt::format(defFormat, p.first, flen, p.second, mlen);
  }

  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat, "TOTAL", flen, mutables.size(), mlen);
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);

  // ── Footer ──────────────────────────────────────────────────────────────────
  std::size_t numFiles = groupByPath.size();
  std::size_t candidateCount = mGenerator->getCandidateCount();
  if (!generatorStr.empty()) {
    std::cout << fmt::format("Generator : {}  |  Seed: {}\n", generatorStr, randomSeed);
  }
  std::cout << fmt::format("Analyzed  : {} source line{} across {} file{}\n",
                           sourceLines.size(), sourceLines.size() == 1 ? "" : "s",
                           numFiles, numFiles == 1 ? "" : "s");
  std::cout << fmt::format("Selected  : {} out of {} candidate{}\n",
                           mutables.size(), candidateCount, candidateCount == 1 ? "" : "s");
  std::cout << fmt::format("{0:=^{1}}\n", "", maxlen);

  return mutables;
}

Mutants MutationFactory::populate(const std::string& gitPath, const SourceLines& sourceLines, std::size_t maxMutants) {
  return populate(gitPath, sourceLines, maxMutants, std::random_device {}(), "");
}

}  // namespace sentinel
