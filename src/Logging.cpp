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
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <string>
#include "sentinel/Logging.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"


namespace sentinel {

std::shared_ptr<Logging> Logging::getLogger(const std::string& name) {
  return getLogger(name, "{name}:{level}: {message}");
}

std::shared_ptr<Logging> Logging::getLogger(const std::string& name,
                                            const std::string& formatter) {
  return std::shared_ptr<Logging>(new Logging(name, formatter));
}

Logging::Logging(const std::string& name, const std::string& formatter) :
    mName(name), mFormatter(formatter), mLevel(Level::INFO) {
  try {
    format(mLevel, mName);
  }
  catch (const fmt::format_error& e) {
    throw InvalidArgumentException(e.what());
  }
}

void Logging::setLevel(Logging::Level level) {
  mLevel = level;
}

std::string Logging::format(Logging::Level level,
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
  return fmt::format(mFormatter,
                     fmt::arg("name", mName),
                     fmt::arg("level", levelText),
                     fmt::arg("message", message));
}

bool Logging::isAllowed(Logging::Level level) {
  return static_cast<int>(mLevel) <= static_cast<int>(level);
}

void Logging::debug(const std::string& message) {
  auto level = Level::DEBUG;
  if (isAllowed(level)) {
    std::cout << format(level, message) << std::endl;
  }
}

void Logging::info(const std::string& message) {
  auto level = Level::INFO;
  if (isAllowed(level)) {
    std::cout << format(level, message) << std::endl;
  }
}

void Logging::warn(const std::string& message) {
  auto level = Level::WARN;
  if (isAllowed(level)) {
    std::cerr << format(level, message) << std::endl;
  }
}

void Logging::error(const std::string& message) {
  auto level = Level::ERROR;
  if (isAllowed(level)) {
    std::cerr << format(level, message) << std::endl;
  }
}

}  // namespace sentinel
