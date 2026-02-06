/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <exception>
#include <memory>
#include <string>
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"

namespace sentinel {

std::shared_ptr<MutantGenerator> MutantGenerator::getInstance(const std::string& generator,
                                                              const std::string& directory) {
  if (generator == "uniform") {
    return std::make_shared<UniformMutantGenerator>(directory);
  }
  if (generator == "random") {
    return std::make_shared<RandomMutantGenerator>(directory);
  }
  if (generator == "weighted") {
    return std::make_shared<WeightedMutantGenerator>(directory);
  }
  throw InvalidArgumentException(fmt::format("Invalid value for generator option: {0}", generator));
}

}  // namespace sentinel
