/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONFIGVALIDATOR_HPP_
#define INCLUDE_SENTINEL_CONFIGVALIDATOR_HPP_

#include "sentinel/Config.hpp"

namespace sentinel {

/**
 * @brief Validates a resolved Config before the mutation pipeline starts.
 */
class ConfigValidator {
 public:
  /**
   * @brief Validate required options and warn about risky settings.
   *
   * Throws InvalidArgumentException on hard errors.
   * Prints warnings and calls Console::confirm() for soft issues
   * unless config.force is true.
   *
   * @param config Fully resolved configuration.
   * @return true to continue, false if the user declined a warning prompt.
   */
  static bool validate(const Config& config);

 private:
  static bool checkWarnings(const Config& config);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CONFIGVALIDATOR_HPP_
