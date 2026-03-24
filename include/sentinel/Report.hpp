/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_REPORT_HPP_
#define INCLUDE_SENTINEL_REPORT_HPP_

#include <filesystem>  // NOLINT
#include "sentinel/MutationSummary.hpp"

namespace sentinel {

/**
 * @brief Abstract base class for mutation report views.
 *
 * Subclasses implement save() to render the report in a specific format.
 * printSummary() outputs a human-readable summary to the console.
 */
class Report {
 public:
  /**
   * @brief Construct a report view from a precomputed summary.
   *
   * @param summary Aggregated mutation data to render.
   */
  explicit Report(const MutationSummary& summary);

  /**
   * @brief Default Destructor
   */
  virtual ~Report() = default;

  /**
   * @brief Save the report to the given path.
   *
   * @param path Directory to save the report into.
   */
  virtual void save(const std::filesystem::path& path) = 0;

  /**
   * @brief Print a human-readable summary to the console.
   */
  void printSummary() const;

 protected:
  MutationSummary mSummary;  ///< Aggregated mutation data
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_REPORT_HPP_
