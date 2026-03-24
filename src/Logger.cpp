/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include "sentinel/Logger.hpp"

namespace sentinel {

static Logger::Level defaultLevel = Logger::Level::INFO;

void Logger::setLevel(Logger::Level level) {
  defaultLevel = level;
}

bool Logger::isAllowed(Logger::Level level) {
  return static_cast<int>(defaultLevel) <= static_cast<int>(level);
}

}  // namespace sentinel
