/*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

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
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"


namespace sentinel {

static std::map<std::string, std::shared_ptr<Logger>> loggers;
static Logger::Level defaultLevel = Logger::Level::OFF;


std::shared_ptr<Logger> Logger::getLogger(const std::string& name) {
  return getLogger(name, "{name} [{level}] {message}");
}

std::shared_ptr<Logger> Logger::getLogger(const std::string& name,
                                          const std::string& format) {
  loggers.emplace(name, std::shared_ptr<Logger>(new Logger(name,
                                                           format,
                                                           defaultLevel)));
  return loggers.at(name);
}

void Logger::setDefaultLevel(Logger::Level level) {
  defaultLevel = level;
}

Logger::Logger(const std::string& name,
               const std::string& format,
               Logger::Level level) :
    mName(name), mFormat(format), mLevel(level) {
  try {
    this->format(mLevel, mName);
  }
  catch (const fmt::format_error& e) {
    throw InvalidArgumentException(e.what());
  }
}

std::string Logger::format(Logger::Level level,
                           const std::string& message) {
  std::string levelText;
  if (level == Level::DEBUG) {
    levelText = "DEBUG";
  } else if (level == Level::INFO) {
    levelText = "INFO";
  } else if (level == Level::WARN) {
    levelText = "WARN";
  } else if (level == Level::ERROR) {
    levelText = "ERROR";
  } else {
    levelText = "UNKNOWN";
  }
  return fmt::format(mFormat,
                     fmt::arg("name", mName),
                     fmt::arg("level", levelText),
                     fmt::arg("message", message));
}

bool Logger::isAllowed(Logger::Level level) {
  return static_cast<int>(mLevel) <= static_cast<int>(level);
}

void Logger::debug(const std::string& message) {
  auto level = Level::DEBUG;
  if (isAllowed(level)) {
    std::cout << format(level, message) << std::endl;
  }
}

void Logger::info(const std::string& message) {
  auto level = Level::INFO;
  if (isAllowed(level)) {
    std::cout << format(level, message) << std::endl;
  }
}

void Logger::warn(const std::string& message) {
  auto level = Level::WARN;
  if (isAllowed(level)) {
    std::cerr << format(level, message) << std::endl;
  }
}

void Logger::error(const std::string& message) {
  auto level = Level::ERROR;
  if (isAllowed(level)) {
    std::cerr << format(level, message) << std::endl;
  }
}

void Logger::setLevel(Logger::Level level) {
  mLevel = level;
}

}  // namespace sentinel
