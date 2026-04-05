/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>
#include <string>
#include "sentinel/Logger.hpp"

namespace sentinel {

static Logger::Level defaultLevel = Logger::Level::INFO;

void Logger::setLevel(Logger::Level level) {
  defaultLevel = level;
}

bool Logger::isAllowed(Logger::Level level) {
  return static_cast<int>(defaultLevel) <= static_cast<int>(level);
}

bool Logger::isTty() {
  static const bool tty = isatty(STDERR_FILENO);
  return tty;
}

void Logger::logMessage(const char* color, const std::string& msg) {
  if (isTty()) {
    Console::err("{}{}{}", color, msg, kColorReset);
  } else {
    Console::err("{}", msg);
  }
}

}  // namespace sentinel
