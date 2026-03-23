/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_EXCEPTIONS_THRESHOLDERROR_HPP_
#define INCLUDE_SENTINEL_EXCEPTIONS_THRESHOLDERROR_HPP_

#include <fmt/core.h>
#include <stdexcept>

namespace sentinel {

/**
 * @brief Thrown when the mutation score falls below the configured threshold.
 */
class ThresholdError : public std::runtime_error {
 public:
  /**
   * @brief Constructor.
   * @param score     Actual mutation score (percentage).
   * @param threshold Required minimum score (percentage).
   */
  ThresholdError(double score, double threshold) :
      std::runtime_error(fmt::format("Mutation score {:.1f}% is below threshold {:.1f}%", score, threshold)) {
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EXCEPTIONS_THRESHOLDERROR_HPP_
