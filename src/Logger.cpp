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
  loggers.emplace(name, std::shared_ptr<Logger>(new Logger(name, defaultLevel)));
  return loggers.at(name);
}

Logger::Logger(const std::string& name, Logger::Level level) : mName(name), mLevel(level) {
}

void Logger::setDefaultLevel(Logger::Level level) {
  std::lock_guard<std::mutex> lock(loggersMutex);
  defaultLevel = level;
}

void Logger::clearCache() {
  std::lock_guard<std::mutex> lock(loggersMutex);
  loggers.clear();
}

bool Logger::isAllowed(Logger::Level level) {
  return static_cast<int>(mLevel) <= static_cast<int>(level);
}

void Logger::setLevel(Logger::Level level) {
  mLevel = level;
}

}  // namespace sentinel
