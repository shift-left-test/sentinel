/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include "sentinel/Logger.hpp"

namespace sentinel {

static std::map<std::string, std::shared_ptr<Logger>> loggers;
static Logger::Level defaultLevel = Logger::Level::INFO;
static std::mutex loggersMutex;

std::shared_ptr<Logger> Logger::getLogger(const std::string& name) {
  std::lock_guard<std::mutex> lock(loggersMutex);
  auto [it, inserted] = loggers.try_emplace(name, nullptr);
  if (inserted) {
    it->second.reset(new Logger(name));
  }
  return it->second;
}

Logger::Logger(const std::string& name) : mName(name) {
}

void Logger::setLevel(Logger::Level level) {
  std::lock_guard<std::mutex> lock(loggersMutex);
  defaultLevel = level;
}

void Logger::clearCache() {
  std::lock_guard<std::mutex> lock(loggersMutex);
  loggers.clear();
}

bool Logger::isAllowed(Logger::Level level) {
  return static_cast<int>(defaultLevel) <= static_cast<int>(level);
}

}  // namespace sentinel
