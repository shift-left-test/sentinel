/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <fmt/core.h>
#include <experimental/filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include "sentinel/Logger.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/SourceLines.hpp"


namespace sentinel {

MutationFactory::MutationFactory(
    const std::shared_ptr<MutantGenerator>& generator) :
    mGenerator(generator) {
}

Mutants MutationFactory::populate(const std::string& gitPath,
                                  const SourceLines& sourceLines,
                                  std::size_t maxMutants,
                                  unsigned randomSeed) {
  namespace fs = std::experimental::filesystem;

  auto logger = Logger::getLogger("populate");
  logger->info(fmt::format("random seed: {}", randomSeed));
  Mutants mutables = mGenerator->populate(sourceLines, maxMutants, randomSeed);

  // Count number of mutants generated in each file
  std::map<std::string, std::size_t> groupByPath;
  for (const auto& m : mutables) {
    std::string path = m.getPath();
    auto it = groupByPath.find(path);

    if (it != groupByPath.end()) {
      ++it->second;
    } else {
      groupByPath[path] = 1;
    }
  }

  // Printing summary
  std::size_t flen = 50;
  std::size_t mlen = 10;
  std::size_t maxlen = flen + mlen + 2;
  std::string defFormat = "{0:<{1}}{2:>{3}}\n";
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format("{0:^{1}}\n", "Mutant Population Report", maxlen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat,
                           "File", flen,
                           "#mutation", mlen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  for (const auto& p : groupByPath) {
    std::string filePath = fs::canonical(p.first).string().substr(
        fs::canonical(gitPath).string().length() + 1);

    int filePos = filePath.size() - flen;
    std::string skipStr;
    if (filePos < 0) {
      filePos = 0;
    } else if (filePos > 1) {
      filePos += 4;
      skipStr = "... ";
    }
    std::cout << fmt::format(defFormat,
                             skipStr + filePath.substr(filePos), flen,
                             p.second, mlen);
  }

  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);
  std::cout << fmt::format(defFormat,
                           "TOTAL", flen,
                           mutables.size(), mlen);
  std::cout << fmt::format("{0:-^{1}}\n", "", maxlen);

  logger->info(fmt::format("source lines: {}", sourceLines.size()));
  logger->info(fmt::format("generated mutables count: {}",
                           mutables.size()));

  return mutables;
}

Mutants MutationFactory::populate(const std::string& gitPath,
                                  const SourceLines& sourceLines,
                                  std::size_t maxMutants) {
  return populate(gitPath, sourceLines, maxMutants, std::random_device {}());
}

}  // namespace sentinel
