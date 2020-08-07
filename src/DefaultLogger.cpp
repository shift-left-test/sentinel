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

#include "sentinel/DefaultLogger.hpp"


namespace sentinel {

DefaultLogger::DefaultLogger(const std::string& name) :
    mName(name) {
  mFormatString = "{name}::{type}::{message}";
}

DefaultLogger::DefaultLogger(const std::string& name,
    const std::string& formatString) :
    mName(name),
    mFormatString(formatString) {
  // Exception occurs when formatString has unexpected arguments
  try {
    fmt::format(mFormatString, fmt::arg("name", "ARGCHK"),
      fmt::arg("type", "ARGCHK"), fmt::arg("message", "ARGCHK"));
  }
  catch (const fmt::format_error& e) {
    throw InvalidArgumentException(e.what());
  }
}

void DefaultLogger::error(const std::string& message) {
  std::cerr << fmt::format(mFormatString, fmt::arg("name", mName),
      fmt::arg("type", "ERROR"), fmt::arg("message", message)) << std::endl;
}

void DefaultLogger::info(const std::string& message) {
  std::cout << fmt::format(mFormatString, fmt::arg("name", mName),
      fmt::arg("type", "INFO"), fmt::arg("message", message)) << std::endl;
}

void DefaultLogger::warn(const std::string& message) {
  std::cerr << fmt::format(mFormatString, fmt::arg("name", mName),
      fmt::arg("type", "WARN"), fmt::arg("message", message)) << std::endl;
}

}  // namespace sentinel
