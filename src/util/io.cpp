/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include "sentinel/util/io.hpp"

namespace sentinel::io {

std::string readLastLines(const std::filesystem::path& path, std::size_t n) {
  std::ifstream file(path);
  if (!file) {
    return "";
  }
  std::deque<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(std::move(line));
    if (lines.size() > n) {
      lines.pop_front();
    }
  }
  std::string result;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) result += '\n';
    result += lines[i];
  }
  return result;
}

void appendLogTail(std::string* msg, const std::filesystem::path& logPath,
                   std::size_t n, const std::string& label) {
  std::string tail = readLastLines(logPath, n);
  if (!tail.empty()) {
    static constexpr auto kIndent = "       ";
    std::string header = fmt::format("--- {} (last {} lines) ---", label, n);
    std::string footer(header.size(), '-');

    *msg += "\n\n";
    *msg += kIndent;
    *msg += header;

    std::istringstream stream(tail);
    std::string line;
    while (std::getline(stream, line)) {
      *msg += '\n';
      *msg += kIndent;
      *msg += line;
    }

    *msg += '\n';
    *msg += kIndent;
    *msg += footer;
  }
}

}  // namespace sentinel::io
